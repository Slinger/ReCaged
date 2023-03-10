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

#include "geom.hpp"
#include "collision_feedback.hpp"
#include "event_buffers.hpp"

#include "common/internal.hpp"
#include "common/log.hpp"
#include "common/threads.hpp"

#include "assets/track.hpp"
#include "assets/conf.hpp"

#include <ode/ode.h>
#include <string.h>

//
//for creation/destruction:
//
Geom *Geom::head = NULL;

//allocates a new geom data, returns its pointer (and uppdate its object's count),
//ads it to the component list, and ads the data to specified geom (assumed)
Geom::Geom (dGeomID geom, Object *obj): Component(obj) //pass object argument to base class constructor
{
	//increase object activity counter
	object_parent->Increase_Activity();

	//parent object
	if (obj->selected_space) dSpaceAdd (obj->selected_space, geom);
	else //add geom to global space
		dSpaceAdd (simulation_thread.space, geom);

	//add it to the geom list
	next = Geom::head;
	Geom::head = this;
	prev = NULL;

	//one more geom after this
	if (next) next->prev = this;

	//add it to the geom
	dGeomSetData (geom, (void*)(Geom*)(this));
	geom_id = geom;

	//now lets set some default values...
	//event processing (triggering):
	colliding = false; //no collision event yet
	model = NULL; //default: don't render

	//special geom indicators
	wheel = NULL; //not a wheel (for now)
	triangle_count = 0; //no "triangles"
	material_count = 0; //no "materials"
	triangle_colliding = NULL;
	material_surfaces = NULL;

	//events:
	//for force handling (disable)
	buffer_event=false;
	force_to_body=NULL; //when true, points at wanted body
	sensor_event=false;

	//debug variables
	flipper_geom = 0;
	TMP_pillar_geom =false; //not a demo pillar geom
}
//destroys a geom, and removes it from the list
Geom::~Geom ()
{
	//remove all events
	Event_Buffer_Remove_All(this);

	//1: remove it from the list
	if (!prev) //head in list, change head pointer
		Geom::head = next;
	else //not head in list, got a previous link to update
		prev->next = next;

	if (next) //not last link in list
		next->prev = prev;

	//remove actual geom from ode
	dGeomDestroy(geom_id);

	//if wheel data, remove to
	delete wheel;

	//decrease activity and check if 0
	object_parent->Decrease_Activity();

	//clear possible collision checking and material-based-surfaces
	delete[] triangle_colliding;
	delete[] material_surfaces;
}

//set defaults:
Surface::Surface()
{
	//collision contactpoint data
	mu = 0.0;
	spring = dInfinity; //infinite spring constant (disabled)
	damping = dInfinity; //ignore damping from this surface (only used with spring)
	bounce = 0.0; //no bouncyness

	//friction scaling for tyre
	sensitivity = 1.0;
	rollres = 1.0;
}

//
//for collisions:
//
Surface *Geom::Find_Material_Surface(const char *name)
{
	//is possible at all?
	if (!material_count)
	{
		Log_Add(0, "WARNING: tried to use per-material surfaces for non-trimesh geom");
		return NULL;
	}

	//firtst of all, check if enabled?
	if (!material_surfaces)
	{
		Log_Add(2, "enabling per-material surfaces for trimesh geom");
		material_surfaces = new Surface[material_count];

		//set default (set to out global surface)
		for (int i=0; i<material_count; ++i)
			material_surfaces[i] = surface;
	}

	//ok 
	int i;
	for (i=0; i<material_count && strcmp(name, parent_materials[i].name); ++i);

	if (i==material_count)
	{
		Log_Add(0, "WARNING: could not find material \"%s\" for trimesh", name);
		return NULL;
	}
	else
		return &material_surfaces[i];
}

//clears geom collision flags
void Geom::Clear_Collisions()
{
	for (Geom *geom=head; geom; geom=geom->next)
	{
		geom->colliding = false;

		//clear all triangle collision bools (for trimeshes)
		if (geom->triangle_count)
			memset(geom->triangle_colliding, false, sizeof(bool)*geom->triangle_count);
	}
}

//when two geoms might intersect
void Geom::Collision_Callback (void *data, dGeomID o1, dGeomID o2)
{
	//check if one (or both) geom is space
	if (dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		dSpaceCollide2 (o1,o2, data, &Collision_Callback);
		return;
	}

	//get attached bodies
	dBodyID b1, b2;
	b1 = dGeomGetBody(o1);
	b2 = dGeomGetBody(o2);

	//the same body, and if both are NULL (no bodies at all)
	//(should probably add a gotgeom/gotnogeom bitfield to also skip hashing)
	//TODO: bitfield indication for static geoms (without body)!"
	if (b1 == b2)
		return; //stop

	//both geoms are geoms, get Geom (metadatas) for/from geoms
	Geom *geom1, *geom2;
	geom1 = (Geom*) dGeomGetData (o1);
	geom2 = (Geom*) dGeomGetData (o2);

	//pointer to the surface settings of both geoms
	Surface *surf1, *surf2;

	//the stepsize (supplied as the "collision data")
	dReal stepsize = *((dReal*)data);

	dContact contact[internal.contact_points];
	int count = dCollide (o1,o2,internal.contact_points, &contact[0].geom, sizeof(dContact));

	//if returned 0 collisions (did not collide), stop
	if (count == 0)
		return;

	//might be needed
	int mcount;

	//OR: do this for all geoms? this is cheapest and most important
	bool wheel1=false, wheel2=false;
	if (geom1->wheel)
	{
		if (!geom2->wheel && b1)
			wheel1=true;
	}
	else if (geom2->wheel && b2)
		wheel2=true;

	//store wheel axle direction right once (instead of querying again)
	dReal wheelaxle[3];
	if (wheel1)
	{
		const dReal *rot = dBodyGetRotation(b1);
		wheelaxle[0] = rot[2];
		wheelaxle[1] = rot[6];
		wheelaxle[2] = rot[10];
	}
	else if (wheel2)
	{
		const dReal *rot = dBodyGetRotation(b2);
		wheelaxle[0] = rot[2];
		wheelaxle[1] = rot[6];
		wheelaxle[2] = rot[10];
	}

	//loop through all collision points and configure surface settings for each
	for (int i=0; i<count; ++i)
	{
		//use the geom's global surface values for now...
		surf1 = &geom1->surface;
		surf2 = &geom2->surface;

		//check if trimeshes
		//using the side{1,2} values: are the triangle indices (not documented feature in ode...)
		if (geom1->triangle_count) //is trimesh with per-triangle enabled
		{
			//might have index value of -1. shouldn't really hapen, but check anyway
			if (contact[i].geom.side1 != -1)
			{
				//set collision flag for this triangle
				geom1->triangle_colliding[contact[i].geom.side1] = true;

				//surface based on material?
				if (geom1->material_surfaces)
				{
					//loop through all materials until finding the one for this triangle
					for (mcount=0; mcount<geom1->material_count &&
							!(contact[i].geom.side1 < geom1->parent_materials[mcount].end);
							++mcount);

					//set surface
					surf1 = &geom1->material_surfaces[mcount];
				}
			}
		}
		if (geom2->triangle_count) //the same for the other
		{
			//might have index value of -1. shouldn't really hapen, but check anyway
			if (contact[i].geom.side2 != -1)
			{
				//set collision flag for this triangle
				geom2->triangle_colliding[contact[i].geom.side2] = true;

				if (geom2->material_surfaces)
				{
					for (mcount=0; mcount<geom2->material_count &&
							!(contact[i].geom.side2 < geom2->parent_materials[mcount].end);
							++mcount);

					surf2 = &geom2->material_surfaces[mcount];
				}
			}
		}

		//as long as one geom got a spring of not 0, it should trigger the other
		if (surf1->spring) //geom1 would have generated collision
			geom2->colliding = true; //thus geom2 is colliding

		if (surf2->spring) //geom2 would have generated collision
			geom1->colliding = true; //thus geom2 is colliding


		//does both components want to collide for real? (not "ghosts"/"sensors")
		//if any geom got a spring of 0, it doesn't want/need to collide:
		//TODO: bitfield indication for sensors (geoms with spring=0)!
		if (surf1->spring==0 || surf2->spring==0)
			continue; //check next collision/stop checking is last


		//sett surface options:
		//enable mu overriding and good friction approximation
		contact[i].surface.mode = dContactApprox1;

		contact[i].surface.mu = (surf1->mu)*(surf2->mu); //friction

		//optional or not even/rarely used by recaged, set to 0 to prevent compiler warnings:
		contact[i].surface.mu2 = 0.0; //only for tyre
		contact[i].surface.bounce = 0.0;
		contact[i].surface.bounce_vel = 0.0;
		contact[i].surface.motion1 = 0.0; //for conveyor belt?
		contact[i].surface.motion2 = 0.0; //for conveyor belt?
		contact[i].surface.motionN = 0.0; //what _is_ this for?
		contact[i].surface.slip1 = 0.0; //not used
		contact[i].surface.slip2 = 0.0; //not used

		//
		//optional features:
		//
		//optional bouncyness (good for wheels?)
		if (surf1->bounce != 0.0 || surf2->bounce != 0.0)
		{
			//enable bouncyness
			contact[i].surface.mode |= dContactBounce;

			//use sum
			contact[i].surface.bounce = (surf1->bounce)+(surf2->bounce);
			contact[i].surface.bounce_vel = 0.0; //not used by recaged right now, perhaps for future tweaking?
		}

		//optional spring+damping erp+cfm override
		if (surf1->spring != dInfinity || surf2->spring != dInfinity)
		{
			//should be good
			dReal spring = 1/( 1/(surf1->spring) + 1/(surf2->spring) );
			//similar
			dReal damping = 1/( 1/(surf1->damping) + 1/surf2->damping );

			//recalculate erp+cfm from stepsize, spring and damping values:
			contact[i].surface.mode |= dContactSoftERP | dContactSoftCFM; //enable local erp/cfm settings
			contact[i].surface.soft_erp = (stepsize*spring)/(stepsize*spring +damping);
			contact[i].surface.soft_cfm = 1.0/(stepsize*spring +damping);
		}
		//end of optional features

		//
		//simulation of wheel or normal geom?
		//modify contacts according to slip and similar
		//
		//determine if _one_ of the geoms is a wheel
		if (wheel1)
			geom1->wheel->Add_Contact(b1, b2, geom1, geom2, true, wheelaxle, surf2, &contact[i], stepsize); //configures friction values based on slip and similar
		else if (wheel2)
			geom2->wheel->Add_Contact(b1, b2, geom1, geom2, false, wheelaxle, surf1, &contact[i], stepsize);
		//TODO: haven't looked at wheel*wheel collision simulation! (will be rim_mu*rim_mu for tyre right now)
		//if (geom1->wheel&&geom2->wheel)
		//	?...
		else
		{
			//create the contactjoints for normal collisions (not wheels)
			dJointID c = dJointCreateContact (simulation_thread.world,simulation_thread.contactgroup,&contact[i]);
			dJointAttach (c,b1,b2);

			//if any of the geoms responds to forces or got a body that responds to force, enable force feedback
			if (geom1->buffer_event || geom2->buffer_event || geom1->force_to_body || geom2->force_to_body)
				new Collision_Feedback(c, geom1, geom2);
		}
	}
}

//
//set events:
//

//buffer
void Geom::Set_Buffer_Event(dReal thres, dReal buff, Script *scr)
{
	if (thres > 0 && buff > 0 && scr)
	{
		threshold=thres;
		buffer=buff;
		buffer_script=scr;

		//make sure no old event is left
		Event_Buffer_Remove_All(this);

		buffer_event=true;
	}
	else
	{
		buffer_event=false;
		Event_Buffer_Remove_All(this);
	}
}

void Geom::Set_Buffer_Body(Body *b)
{
	force_to_body = b;
}

void Geom::Damage_Buffer(dReal force, dReal step)
{
	if (force_to_body)
	{
		force_to_body->Damage_Buffer(force, step);
		return;
	}

	//if not configured or force not exceeds threshold, stop
	if (!buffer_event || (force<threshold))
		return;

	//buffer still got health
	if (buffer > 0)
	{
		buffer -= force*step;

		//now it's negative, issue event
		if (buffer < 0)
			Event_Buffer_Add_Depleted(this);
	}
	else //just damage buffer even more
		buffer -= force*step;
}

void Geom::Increase_Buffer(dReal buff)
{
	buffer+=buff;

	if (buffer < 0) //still depleted, regenerate event
		Event_Buffer_Add_Depleted(this);
}

//sensor
void Geom::Set_Sensor_Event(Script *s1, Script *s2)
{
	if (s1 || s2) //enable
	{
		sensor_triggered_script=s1;
		sensor_untriggered_script=s2;
		sensor_last_state=false;
		sensor_event=true;
	}
	else //disable
		sensor_event=false;
}

//physics step
void Geom::Physics_Step()
{
	Geom *geom;
	for (geom=head; geom; geom=geom->next)
	{
		if (geom->sensor_event)
		{
			//triggered/untriggered
			if (geom->colliding != geom->sensor_last_state)
			{
				geom->sensor_last_state=geom->colliding;
				Event_Buffer_Add_Triggered(geom);
			}
		}

		//if (geom->radar_event)... - TODO
	}
}
