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

#include "body.hpp"
#include "common/log.hpp"
#include "common/internal.hpp"
#include "assets/object.hpp"
#include "assets/car.hpp"
#include "assets/track.hpp"
#include "event_buffers.hpp"

//for creation:
Body *Body::head = NULL;

Body::Body (dBodyID body, Object *obj): Component(obj)
{
	//increase object activity counter
	object_parent->Increase_Activity();

	//ad it to the list
	next = head;
	head = this;
	prev = NULL;

	if (next) next->prev = this;

	//add it to the body
	dBodySetData (body, (void*)(this));
	body_id = body;

	//default values
	model = NULL; //don't render
	Update_Mass(); //get current mass...
	Set_Linear_Drag(internal.linear_drag); //...and set up drag
	Set_Angular_Drag(internal.angular_drag);//...
	buffer_event=false; //no events yet
}

//destroys a body, and removes it from the list
Body::~Body()
{
	//remove all events
	Event_Buffer_Remove_All(this);

	//1: remove it from the list
	if (!prev) //head in list, change head pointer
		head = next;
	else //not head in list, got a previous link to update
		prev->next = next;

	if (next) //not last link in list
		next->prev = prev;

	//2: remove it from memory

	dBodyDestroy(body_id);

	//decrease activity and check if 0
	object_parent->Decrease_Activity();
}

//for physics:
#define v_length(x, y, z) (sqrt( (x)*(x) + (y)*(y) + (z)*(z) ))
//functions for body drag

void Body::Update_Mass()
{
	dMass dmass;

	//TODO: use the body's inertia tensor instead...?
	dBodyGetMass (body_id, &dmass);

	mass = dmass.mass;
}

//NOTE: modifying specified drag to the current mass (rice-burning optimization, or actually good idea?)
//(this way the body mass doesn't need to be requested and used in every calculation)
void Body::Set_Linear_Drag (dReal drag)
{
	linear_drag[0] = drag;
	use_axis_linear_drag = false;
}

void Body::Set_Axis_Linear_Drag (dReal drag_x, dReal drag_y, dReal drag_z)
{
	linear_drag[0] = drag_x;
	linear_drag[1] = drag_y;
	linear_drag[2] = drag_z;

	use_axis_linear_drag = true;
}

void Body::Set_Angular_Drag (dReal drag)
{
	angular_drag[0] = drag;
	use_axis_angular_drag = false;
}

void Body::Set_Axis_Angular_Drag (dReal drag_x, dReal drag_y, dReal drag_z)
{
	angular_drag[0] = drag_x;
	angular_drag[1] = drag_y;
	angular_drag[2] = drag_z;

	use_axis_angular_drag = true;
}


//simulation of drag
//
//not to self: if implementing different density areas, this is where density should be chosen
void Body::Linear_Drag (dReal step)
{
	const dReal *abs_vel; //absolute vel
	abs_vel = dBodyGetLinearVel (body_id);
	dReal vel[3] = {abs_vel[0]-track.wind[0], abs_vel[1]-track.wind[1], abs_vel[2]-track.wind[2]}; //relative to wind
	dReal total_vel = v_length(vel[0], vel[1], vel[2]);

	//how much of original velocity is left after braking by air/liquid drag
	dReal scale=1.0/(1.0+total_vel*(track.density)*(linear_drag[0]/mass)*(step));

	//change velocity
	vel[0]*=scale;
	vel[1]*=scale;
	vel[2]*=scale;

	//make absolute
	vel[0]+=track.wind[0];
	vel[1]+=track.wind[1];
	vel[2]+=track.wind[2];

	//set velocity
	dBodySetLinearVel(body_id, vel[0], vel[1], vel[2]);
}

//similar to linear_drag, but different drag for different (local) directions
void Body::Axis_Linear_Drag (dReal step)
{
	//absolute velocity
	const dReal *abs_vel;
	abs_vel = dBodyGetLinearVel (body_id);

	//translate movement to relative to car (and to wind)
	dVector3 vel;
	dBodyVectorFromWorld (body_id, (abs_vel[0]-track.wind[0]), (abs_vel[1]-track.wind[1]), (abs_vel[2]-track.wind[2]), vel);
	dReal total_vel = v_length(vel[0], vel[1], vel[2]);

	//how much of original velocities is left after braking by air/liquid drag
	vel[0]/=1.0+(total_vel*(track.density)*(linear_drag[0]/mass)*(step));
	vel[1]/=1.0+(total_vel*(track.density)*(linear_drag[1]/mass)*(step));
	vel[2]/=1.0+(total_vel*(track.density)*(linear_drag[2]/mass)*(step));

	//make absolute
	dVector3 vel_result;
	dBodyVectorToWorld (body_id, vel[0], vel[1], vel[2], vel_result);

	//add wind
	vel_result[0]+=track.wind[0];
	vel_result[1]+=track.wind[1];
	vel_result[2]+=track.wind[2];

	//set velocity
	dBodySetLinearVel(body_id, vel_result[0], vel_result[1], vel_result[2]);
}

void Body::Angular_Drag (dReal step)
{
	const dReal *vel; //rotation velocity
	vel = dBodyGetAngularVel (body_id);
	dReal total_vel = v_length(vel[0], vel[1], vel[2]);

	//how much of original velocity is left after braking by air/liquid drag
	dReal scale=1.0/(1.0+total_vel*(track.density)*(angular_drag[0]/mass)*(step));

	//set velocity with change
	dBodySetAngularVel(body_id, vel[0]*scale, vel[1]*scale, vel[2]*scale);
}

void Body::Axis_Angular_Drag (dReal step)
{
	//rotation velocity
	const dReal *vel = dBodyGetAngularVel (body_id);

	//rotation matrix
	const dReal *rot=dBodyGetRotation(body_id);

	//transform rotation to relative body
	//(could use matrix for drag, like inertia tensor)
	dReal rel[3]={
		rot[0]*vel[0]+rot[4]*vel[1]+rot[8]*vel[2],
		rot[1]*vel[0]+rot[5]*vel[1]+rot[9]*vel[2],
		rot[2]*vel[0]+rot[6]*vel[1]+rot[10]*vel[2] };

	dReal total_vel = v_length(rel[0], rel[1], rel[2]);

	//how much of original velocity is left after braking by air/liquid drag
	rel[0]/=(1.0+total_vel*(track.density)*(angular_drag[0]/mass)*(step));
	rel[1]/=(1.0+total_vel*(track.density)*(angular_drag[1]/mass)*(step));
	rel[2]/=(1.0+total_vel*(track.density)*(angular_drag[2]/mass)*(step));

	//set velocity with change (transformed back to world coordinates)
	dBodySetAngularVel(body_id,
		rot[0]*rel[0]+rot[1]*rel[1]+rot[2]*rel[2],
		rot[4]*rel[0]+rot[5]*rel[1]+rot[6]*rel[2],
		rot[8]*rel[0]+rot[9]*rel[1]+rot[10]*rel[2] );
}

void Body::Set_Buffer_Event(dReal thres, dReal buff, Script *scr)
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
		Event_Buffer_Remove_All(this);

		//disable
		buffer_event=false;
	}
}

void Body::Damage_Buffer(dReal force, dReal step)
{
	//if not processing forces or not high enough force, no point continuing
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

void Body::Physics_Step (dReal step)
{
	for (Body *body=Body::head; body; body=body->next)
	{
		//drag
		if (body->use_axis_linear_drag)
			body->Axis_Linear_Drag(step);
		else //simple drag instead
			body->Linear_Drag(step);

		//angular
		if (body->use_axis_angular_drag)
			body->Axis_Angular_Drag(step);
		else
			body->Angular_Drag(step);
	}
}

