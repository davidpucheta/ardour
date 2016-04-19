/*
    Copyright (C) 2016 Paul Davis

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __libardour_slavable_automation_control_h__
#define __libardour_slavable_automation_control_h__

#include "pbd/enumwriter.h"

#include "ardour/slavable_automation_control.h"
#include "ardour/session.h"

using namespace std;
using namespace ARDOUR;
using namespace PBD;

SlavableAutomationControl::SlavableAutomationControl(ARDOUR::Session& s,
                                                     const Evoral::Parameter&                  parameter,
                                                     const ParameterDescriptor&                desc,
                                                     boost::shared_ptr<ARDOUR::AutomationList> l,
                                                     const std::string&                        name)
	: AutomationControl (s, parameter, desc, l, name)
{
}

double
SlavableAutomationControl::get_masters_value_locked () const
{
	double v = _desc.normal;

	if (_desc.toggled) {
		for (Masters::const_iterator mr = _masters.begin(); mr != _masters.end(); ++mr) {
			if (mr->second.master()->get_value()) {
				return _desc.upper;
			}
		}
		return _desc.lower;
	}

	for (Masters::const_iterator mr = _masters.begin(); mr != _masters.end(); ++mr) {
		/* get current master value, scale by our current ratio with that master */
		v *= mr->second.master()->get_value () * mr->second.ratio();
	}

	return min ((double) _desc.upper, v);
}

double
SlavableAutomationControl::get_value_locked() const
{
	/* read or write masters lock must be held */

	if (_masters.empty()) {
		return Control::get_double (false, _session.transport_frame());
	}

	if (_desc.toggled) {
		/* for boolean/toggle controls, if this slave OR any master is
		 * enabled, this slave is enabled. So check our own value
		 * first, because if we are enabled, we can return immediately.
		 */
		if (Control::get_double (false, _session.transport_frame())) {
			return _desc.upper;
		}
	}

	return get_masters_value_locked ();
}

/** Get the current effective `user' value based on automation state */
double
SlavableAutomationControl::get_value() const
{
	bool from_list = _list && boost::dynamic_pointer_cast<AutomationList>(_list)->automation_playback();

	if (!from_list) {
		Glib::Threads::RWLock::ReaderLock lm (master_lock);
		return get_value_locked ();
	} else {
		return Control::get_double (from_list, _session.transport_frame());
	}
}

void
SlavableAutomationControl::actually_set_value (double val, Controllable::GroupControlDisposition group_override)
{
	val = std::max (std::min (val, (double)_desc.upper), (double)_desc.lower);

	{
		Glib::Threads::RWLock::WriterLock lm (master_lock);

		if (!_masters.empty()) {
			recompute_masters_ratios (val);
		}
	}

	/* this sets the Evoral::Control::_user_value for us, which will
	   be retrieved by AutomationControl::get_value ()
	*/
	AutomationControl::actually_set_value (val, group_override);

	_session.set_dirty ();
}

void
SlavableAutomationControl::add_master (boost::shared_ptr<AutomationControl> m)
{
	double current_value;
	double new_value;
	std::pair<Masters::iterator,bool> res;

	{
		Glib::Threads::RWLock::WriterLock lm (master_lock);
		current_value = get_value_locked ();

		/* ratio will be recomputed below */

		res = _masters.insert (make_pair<PBD::ID,MasterRecord> (m->id(), MasterRecord (m, 1.0)));

		if (res.second) {

			if (_desc.toggled) {
				recompute_masters_ratios (current_value);
			}

			/* note that we bind @param m as a weak_ptr<AutomationControl>, thus
			   avoiding holding a reference to the control in the binding
			   itself.
			*/

			m->DropReferences.connect_same_thread (masters_connections, boost::bind (&SlavableAutomationControl::master_going_away, this, m));

			/* Store the connection inside the MasterRecord, so that when we destroy it, the connection is destroyed
			   and we no longer hear about changes to the AutomationControl.

			   Note that we fix the "from_self" argument that will
			   be given to our own Changed signal to "false",
			   because the change came from the master.
			*/

			m->Changed.connect_same_thread (res.first->second.connection, boost::bind (&SlavableAutomationControl::master_changed, this, _1, _2));
			cerr << this << enum_2_string ((AutomationType) _parameter.type()) << " now listening to Changed from " << m << endl;
		}

		new_value = get_value_locked ();
	}

	if (res.second) {
		/* this will notify everyone that we're now slaved to the master */
		MasterStatusChange (); /* EMIT SIGNAL */
	}

	if (new_value != current_value) {
		/* need to do this without a writable() check in case
		 * the master is removed while this control is doing
		 * automation playback.
		 */
		 actually_set_value (new_value, Controllable::NoGroup);
	}

}

void
SlavableAutomationControl::master_changed (bool /*from_self*/, GroupControlDisposition gcd)
{
	/* our value has (likely) changed, but not because we were
	 * modified. Just the master.
	 */

	/* propagate master state into our own control so that if we stop
	 * being slaved, our value doesn't change, and propagate to any
	 * group this control is part of.
	 */

	cerr << this << ' ' << enum_2_string ((AutomationType) _parameter.type()) << " pass along " << get_masters_value() << " from master to group\n";
	actually_set_value (get_masters_value(), Controllable::UseGroup);
}

void
SlavableAutomationControl::master_going_away (boost::weak_ptr<AutomationControl> wm)
{
	boost::shared_ptr<AutomationControl> m = wm.lock();
	if (m) {
		remove_master (m);
	}
}

void
SlavableAutomationControl::remove_master (boost::shared_ptr<AutomationControl> m)
{
	double current_value;
	double new_value;
	bool masters_left;
	Masters::size_type erased = 0;

	{
		Glib::Threads::RWLock::WriterLock lm (master_lock);
		current_value = get_value_locked ();
		erased = _masters.erase (m->id());
		if (erased) {
			recompute_masters_ratios (current_value);
		}
		masters_left = _masters.size ();
		new_value = get_value_locked ();
	}

	if (erased) {
		MasterStatusChange (); /* EMIT SIGNAL */
	}

	if (new_value != current_value) {
		if (masters_left == 0) {
			/* no masters left, make sure we keep the same value
			   that we had before.
			*/
			actually_set_value (current_value, Controllable::UseGroup);
		}
	}
}

void
SlavableAutomationControl::clear_masters ()
{
	double current_value;
	double new_value;
	bool had_masters = false;

	{
		Glib::Threads::RWLock::WriterLock lm (master_lock);
		current_value = get_value_locked ();
		if (!_masters.empty()) {
			had_masters = true;
		}
		_masters.clear ();
		new_value = get_value_locked ();
	}

	if (had_masters) {
		MasterStatusChange (); /* EMIT SIGNAL */
	}

	if (new_value != current_value) {
		Changed (false, Controllable::NoGroup);
	}

}

bool
SlavableAutomationControl::slaved_to (boost::shared_ptr<AutomationControl> m) const
{
	Glib::Threads::RWLock::ReaderLock lm (master_lock);
	return _masters.find (m->id()) != _masters.end();
}

bool
SlavableAutomationControl::slaved () const
{
	Glib::Threads::RWLock::ReaderLock lm (master_lock);
	return !_masters.empty();
}

#endif /* __libardour_slavable_automation_control_h__ */
