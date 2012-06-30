/*
 * ReCaged - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2009, 2010, 2011 Mats Wahlberg
 *
 * This file is part of ReCaged.
 *
 * ReCaged is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReCaged is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ReCaged.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#ifndef _RC_OBJECT_H
#define _RC_OBJECT_H
#include <ode/common.h>

#include "racetime_data.hpp"
#include "trimesh.hpp"
#include "component.hpp"
#include "space.hpp"

//object: one "thing" on the track, from a complex building to a tree, spawning
//will be controlled by a custom scripting langue in future versions, the most
//important role of "object" is to store the ode space for the spawned object
//(keeps track of the geoms in ode that describes the components) and joint
//group (for cleaning up object)

//template for spawning
class Object_Template:public Racetime_Data
{
	public:
		static Object_Template *Load(const char *path);
		class Object *Spawn(dReal x, dReal y, dReal z);

	private:
		Object_Template(const char*); //just set some default values
		~Object_Template(); //destructor (TODO: virtual?)

		//lua script to be run when spawning object
		int spawn_script;
};

//can be added/removed at runtime ("racetime")
class Object
{
	public:
		virtual ~Object(); //(virtual makes sure also inherited classes calls this destructor)
		static void Destroy_All();

		//for increasing/decreasing activity counter
		void Increase_Activity();
		void Decrease_Activity();
		int Run(int args, int results);
	private:
		Object(dReal pos[3], dReal rot[9]);
		//the following are either using or inherited from this class
		friend class Object_Template; //needs access to constructor
		friend bool load_track (const char *);
		friend class Car;

		//things to keep track of when cleaning out object
		unsigned int activity; //counts geoms,bodies and future stuff (script timers, loops, etc)

		dReal pos[3], rot[9];
		Component *components;
		dSpaceID selected_space; //TODO: remove!

		//to allow acces to the two above pointers
		friend class Component; //components
		friend class Geom; //selected space
		friend class Space; //selected space

		//placeholder for more data

		//TODO: the list of objects and "selected" should be split into specific
		//simulation threads. Only a problem when enabling multiple threads...

		//used to find next/prev object in dynamically allocated chain
		//set next to null in last object in chain
		static Object *head;
		Object *prev, *next;

		static Object *selected;
};

#endif
