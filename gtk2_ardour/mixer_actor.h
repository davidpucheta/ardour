/*
    Copyright (C) 2000 Paul Davis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __gtk2_ardour_mixer_actor_h__
#define __gtk2_ardour_mixer_actor_h__

#include <glibmm/refptr.h>
#include <gtkmm2ext/bindings.h>

#include "route_processor_selection.h"

namespace Gtk {
	class ActionGroup;
}

namespace ARDOUR {
	class VCA;
}

class MixerActor : virtual public sigc::trackable
{
  public:
	MixerActor ();
	virtual ~MixerActor ();

	RouteProcessorSelection& selection() { return _selection; }
	void register_actions ();

        void load_bindings ();
        Gtkmm2ext::Bindings*  bindings;

  protected:
	Gtkmm2ext::ActionMap myactions;
	RouteProcessorSelection _selection;
	RouteUISelection _route_targets;

	virtual void set_route_targets_for_operation () = 0;

	void vca_assign (boost::shared_ptr<ARDOUR::VCA>);
	void vca_unassign (boost::shared_ptr<ARDOUR::VCA>);

	void solo_action ();
	void mute_action ();
	void rec_enable_action ();
	void step_gain_up_action ();
	void step_gain_down_action ();
	void unity_gain_action ();

	void copy_processors ();
	void cut_processors ();
	void paste_processors ();
	void select_all_processors ();
	void toggle_processors ();
	void ab_plugins ();

	//this op is different because it checks _all_ mixer strips, and deletes selected plugins on any of them (ignores track selections)
	//BUT... note that we have used mixerstrip's "Enter" to enforce the rule that only one strip will have an active selection
	virtual void delete_processors () = 0;

	virtual void select_none () = 0;


        /* these actions need access to a Session, do defer to
	   a derived class
	*/
        virtual void toggle_midi_input_active (bool flip_others) = 0;

	/* these actions don't apply to the selection, so defer to
	   a derived class.
	*/
	virtual void scroll_left () {}
	virtual void scroll_right () {}
};

#endif /* __gtk2_ardour_mixer_actor_h__ */
