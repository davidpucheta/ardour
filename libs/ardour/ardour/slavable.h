/*
    Copyright (C) 2016 Paul Davis

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

#ifndef __ardour_slavable_h__
#define __ardour_slavable_h__

#include <set>
#include <string>
#include <stdint.h>

#include <boost/shared_ptr.hpp>

class XMLNode;

namespace ARDOUR {

class VCA;
class Session;

class Slavable
{
    public:
	Slavable ();
	virtual ~Slavable() {}

	XMLNode& state () const;
	int assign (Session& s, XMLNode const&);

	void assign (boost::shared_ptr<VCA>);
	void unassign (boost::shared_ptr<VCA>);

	static std::string xml_node_name;

    protected:
	virtual int assign_controls (boost::shared_ptr<VCA>) = 0;
	virtual int unassign_controls (boost::shared_ptr<VCA>) = 0;

    private:
	mutable Glib::Threads::RWLock master_lock;
	std::set<uint32_t> _masters;
};

} // namespace ARDOUR

#endif /* __ardour_slavable_h__ */