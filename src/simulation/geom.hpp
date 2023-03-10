/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2014, 2015 Mats Wahlberg
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

#ifndef _ReCaged_GEOM_H
#define _ReCaged_GEOM_H
#include <ode/ode.h>
#include "simulation/component.hpp"
#include "simulation/body.hpp"
#include "simulation/wheel.hpp"
#include "assets/object.hpp"
#include "assets/model.hpp"
#include "assets/script.hpp"
#include <SDL/SDL_stdinc.h> //definition for Uint32

//Geom: (meta)data for geometrical shape (for collision detection), for: 
//contactpoint generation (friction and softness/hardness). Also contains
//rendering data for geom

//surface properties:
class Surface
{
	public:
		Surface(); //just sets default values

		//options
		dReal mu, bounce;
		dReal spring, damping;
		dReal sensitivity, rollres;
};


//geom tracking class
class Geom: public Component
{
	public:
		//methods for creating/destroying/processing Geoms
		Geom (dGeomID geom, Object *obj);
		~Geom();

		//methods for steps/simulations:
		static void Clear_Collisions();
		static void Physics_Step();

		static void Collision_Callback(void *, dGeomID, dGeomID);

		//end of methods, variables:
		//geom data bellongs to
		dGeomID geom_id;

		//Physics data:
		Surface surface;

		//placeholder for more physics data

		//register if geom is colliding
		bool colliding; //set after each collision


		//special kind of geoms:
		//wheel: points at a wheel simulation class, or NULL if not a wheel
		class Wheel *wheel;

		//gets surface-per-triangle-per-material
		//also enables this functionality when first run.
		Surface *Find_Material_Surface(const char*);

		//end of special geoms

		//End of physics data
		
		Model_Draw *model; //points at model

		//debug variables
		dGeomID flipper_geom;

		bool TMP_pillar_geom;
		Model_Draw *TMP_pillar_graphics; //TMP

		//for buffer events
		void Set_Buffer_Event(dReal thresh, dReal buff, Script *scr);
		void Increase_Buffer(dReal add);
		void Set_Buffer_Body(Body*); //send damage to body instead
		void Damage_Buffer(dReal force, dReal step); //"damage" geom with specified force

		//sensor events
		void Set_Sensor_Event(Script *s1, Script *s2);

	private:
		//events:
		bool buffer_event;
		bool sensor_event;
		//bool radar_event; - TODO

		//sensor events:
		bool sensor_last_state; //last state of sensor: enabled or disabled
		Script *sensor_triggered_script, *sensor_untriggered_script;

		//buffer events:
		Body *force_to_body; //send forces to this body instead

		//normal buffer handling
		dReal threshold;
		dReal buffer;
		Script *buffer_script; //script to execute when colliding (NULL if not used)

		//for special kind of geoms:
		//trimesh: how many triangles (0 if not trimesh/disabled) and which colliding:
		int triangle_count, material_count;
		bool *triangle_colliding;
		Model_Mesh::Material *parent_materials;
		Surface *material_surfaces;
		//end

		//used to find next/prev geom in list of all geoms
		//set next to null in last link in chain (prev = NULL in first)
		static Geom *head; // = NULL;
		Geom *prev;
		Geom *next;

		friend class Model_Mesh; //will be required to modify triangle_* stuff above
		friend void Render_List_Update(); //to allow loop through geoms
		friend void Event_Buffers_Process(dReal); //to allow looping
		friend void Body::Physics_Step (dReal step); //dito
		friend void Track_Physics_Step();
		friend void Geom_Render(); //same as above, for debug collision render
		friend class Wheel; //to set collision feedbacks
};

#endif
