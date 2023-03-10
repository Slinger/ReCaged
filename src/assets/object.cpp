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

#include <ode/ode.h>
#include <stdlib.h>

#include "object.hpp"
#include "assets.hpp"
#include "model.hpp"
#include "track.hpp"

#include "common/log.hpp"
#include "common/threads.hpp"

#include "simulation/joint.hpp"
#include "simulation/geom.hpp"
#include "simulation/body.hpp"
#include "simulation/event_buffers.hpp"

//allocate new script storage, and add it to list
//(not used yet, only for storing 3d list pointers...)
Module::Module(const char *name): Assets(name)
{
	//debug identification bools set to false
	box = false;
	funbox = false;
	flipper = false;
	NH4 = false;
	building = false;
	sphere = false;
	pillar = false;
	tetrahedron = false;

	//make sure all model pointers are null
	for (int i=0; i<10; ++i)
		model[i]=NULL;

	//and the same for trimesh geoms
	for (int i=0; i<1; ++i)
		geom[i]=NULL;
}


Object *Object::head = NULL;

//allocate a new object, add it to the list and returns its pointer
Object::Object ()
{
	prev=NULL;
	next=head;
	head=this;

	if (next) next->prev = this;

	//default values
	components = NULL;
	activity = 0;
	selected_space = NULL;
}

//destroys an object
Object::~Object()
{
	Log_Add(1, "Object removed");
	//1: remove it from the list
	if (prev == NULL) //first link
		head = next;
	else
		prev->next = next;

	if (next) //not last link
		next->prev = prev;


	//remove components
	while (components)
		delete components; //just removes the one in top each time

	//make sure no events for this object is left
	Event_Buffer_Remove_All(this);
}

//"activity": should be called "references", but don't bother change
void Object::Increase_Activity()
{
	++activity;
}

void Object::Decrease_Activity()
{
	if ((--activity) == 0)
		Event_Buffer_Add_Inactive(this);
}

//destroys all objects
void Object::Destroy_All()
{
	while (head)
		delete (head);
}
//load data for creating object (object data), hard-coded debug version
Module *Module::Load(const char *path)
{
	Log_Add(1, "Loading module: %s", path);

	//see if already loaded
	if (Module *tmp=Assets::Find<Module>(path))
	{
		return tmp;
	}

	//tmp pointers
	Module *obj;
	
	//currently no scripting, only hard-coded solutions
	if (!strcmp(path,"misc/box"))
	{
		//"load" 3d box
		Log_Add(2, "(hard-coded box)");

		obj = new Module(path);
		obj->box = true;

		//the debug box will only need one "3D file"
		if (!(obj->model[0] = Model_Draw::Quick_Load("misc/box/box.obj")))
			return NULL;

	//end of test
	}
	else if (!strcmp(path, "misc/funbox"))
	{
		Log_Add(2, "(Mac's hard-coded funbox");

		obj = new Module(path);
		obj->funbox = true; //id

		//graphics
		if (!(obj->model[0] = Model_Draw::Quick_Load("misc/funbox/box.obj")))
			return NULL;

	}
	else if (!strcmp(path, "misc/flipper"))
	{
		Log_Add(2, "(hard-coded flipper)");

		obj = new Module(path);
		obj->flipper = true; //id

		//graphics
		if (!(obj->model[0] = Model_Draw::Quick_Load("misc/flipper/Flipper.obj")))
			return NULL;

	}
	else if (!strcmp(path, "misc/NH4"))
	{
		Log_Add(2, "(hard-coded \"molecule\")");

		obj = new Module(path);
		obj->NH4 = true;

		//graphics
		if (	!(obj->model[0] = Model_Draw::Quick_Load("misc/NH4/Atom1.obj")) ||
			!(obj->model[1] = Model_Draw::Quick_Load("misc/NH4/Atom2.obj"))	)
			return NULL;

	}
	else if (!strcmp(path, "misc/beachball"))
	{
		Log_Add(2, "(hard-coded beachball)");

		obj = new Module(path);
		obj->sphere = true;
		if (!(obj->model[0] = Model_Draw::Quick_Load("misc/beachball/sphere.obj")))
			return NULL;
	}
	else if (!strcmp(path, "misc/building"))
	{
		Log_Add(2, "(hard-coded building)");

		obj = new Module(path);
		obj->building = true;

		//graphics
		if (	!(obj->model[0] = Model_Draw::Quick_Load("misc/building/pillar.obj")) ||
			!(obj->model[1] = Model_Draw::Quick_Load("misc/building/roof.obj")) ||
			!(obj->model[2] = Model_Draw::Quick_Load("misc/building/wall.obj"))	)
			return NULL;

	}
	else if (!strcmp(path,"misc/pillar"))
	{
		//"load" 3d box
		Log_Add(2, "(hard-coded pillar)");

		obj = new Module(path);
		obj->pillar = true;

		//graphics
		if (	!(obj->model[0] = Model_Draw::Quick_Load("misc/pillar/Pillar.obj")) ||
			!(obj->model[1] = Model_Draw::Quick_Load("misc/pillar/Broken.obj"))	)
			return NULL;

	}
	else if (!strcmp(path,"misc/tetrahedron"))
	{
		//"load" 3d box
		Log_Add(2, "(hard-coded tetrahedron)");

		obj = new Module(path);
		obj->tetrahedron = true;

		//try to load and generate needed vertices (render+collision)
		Model mesh;
		if (	!(mesh.Load("misc/tetrahedron/model.obj")) ||
			!(obj->model[0] = mesh.Create_Draw()) ||
			!(obj->geom[0] = mesh.Create_Mesh())	)
			return NULL;
	}
	else
	{
		Log_Add(-1, "Object path didn't match any hard-coded object");
		return NULL;
	}

	//if we got here, loading ok
	return obj;
}

//bind two bodies together using fixed joint (simplify connection of many bodies)
void debug_joint_fixed(dBodyID body1, dBodyID body2, Object *obj)
{
	dJointID joint;
	joint = dJointCreateFixed (simulation_thread.world, 0);
	Joint *jd = new Joint(joint, obj);
	dJointAttach (joint, body1, body2);
	dJointSetFixed (joint);

	//use feedback
	//set threshold, buffer and dummy script
	jd->Set_Buffer_Event(20000, 5000, (Script*)1337);
}

//create a "loaded" (actually hard-coded) object
//TODO: rotation
void Module::Create (dReal x, dReal y, dReal z)
{
	Log_Add(1, "Creating object at: %f %f %f", x,y,z);
	//pretend to be executing the script... just load debug values
	//

	if (box)
	{
	Log_Add(2, "(hard-coded box)");
	//
	//
	//

	Object *obj = new Object();

	dGeomID geom  = dCreateBox (0, 1,1,1); //geom
	Geom *data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	data->Set_Buffer_Event(100000, 10000, (Script*)1337);
	dBodyID body = dBodyCreate (simulation_thread.world);

	dMass m;
	dMassSetBoxTotal (&m,400,1,1,1); //mass+sides
	dBodySetMass (body, &m);

	new Body(body, obj); //just for drag
	//bd->Set_Event (100, 10, (script_struct*)1337);

	dGeomSetBody (geom, body);

	dBodySetPosition (body, x, y, z);

	//Next, Graphics
	data->model = model[0];

	//done
	}
	//
	//
	else if (funbox)
	{
	Log_Add(2, "(Mac's hard-coded funbox)");
	
	Object *obj = new Object();
	new Space(obj);

	//one body to which all geoms are added
	dBodyID body1 = dBodyCreate (simulation_thread.world);

	//mass
	dMass m;
	dMassSetBoxTotal (&m,100,2.5,2.5,2.5); //mass+sides
	//TODO: create dMass for all geoms, and use dMassTranslate+dMassAdd to add to m
	dBodySetMass (body1, &m);

	//create Body metadata
	Body *bd = new Body (body1, obj);

	//set buffer for body (when destroyed, so are all other geoms)
	bd->Set_Buffer_Event(150000, 10000, (Script*)1337);


	//
	//create geoms
	//

	//geom at (0,0,0)
	dGeomID geom  = dCreateBox (0, 2,2,2); //geom
	Geom *data = new Geom(geom, obj);
	data->surface.mu = 1.0;

	dGeomSetBody (geom, body1);
	dBodySetPosition (body1, x, y, z);

	data->model = model[0];
	data->Set_Buffer_Body(bd); //send collision forces to body
	data->surface.bounce = 2.0; //about twice the collision force is used to bounce up

	//the outer boxes (different offsets)
	geom = dCreateBox(0, 1.0, 1.0, 1.0);
	data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	dGeomSetBody (geom, body1);
	dGeomSetOffsetPosition(geom, 0.70,0,0); //offset
	//data->f_3d = graphics_debug2; //graphics
	data->Set_Buffer_Body(bd);

	geom = dCreateBox(0, 1.0, 1.0, 1.0);
	data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	dGeomSetBody (geom, body1);
	dGeomSetOffsetPosition(geom, 0,0.70,0); //offset
	//data->f_3d = graphics_debug2; //graphics
	data->Set_Buffer_Body(bd);

	geom = dCreateBox(0, 1.0, 1.0, 1.0);
	data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	dGeomSetBody (geom, body1);
	dGeomSetOffsetPosition(geom, 0,0,0.70); //offset
	//data->f_3d = graphics_debug2; //graphics
	data->Set_Buffer_Body(bd);

	geom = dCreateBox(0, 1.0, 1.0, 1.0);
	data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	dGeomSetBody (geom, body1);
	dGeomSetOffsetPosition(geom, -0.70,0,0); //offset
	//data->f_3d = graphics_debug2; //graphics
	data->Set_Buffer_Body(bd);

	geom = dCreateBox(0, 1.0, 1.0, 1.0);
	data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	dGeomSetBody (geom, body1);
	dGeomSetOffsetPosition(geom, 0,-0.70,0); //offset
	//data->f_3d = graphics_debug2; //graphics
	data->Set_Buffer_Body(bd);

	geom = dCreateBox(0, 1.0, 1.0, 1.0);
	data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	dGeomSetBody (geom, body1);
	dGeomSetOffsetPosition(geom, 0,0,-0.70); //offset
	//data->f_3d = graphics_debug2; //graphics
	data->Set_Buffer_Body(bd);
	
	}
	//
	//
	else if (flipper)
	{
	Log_Add(2, "(hard-coded flipper)");
	//
	//
	//

	//flipper surface
	Object *obj = new Object();
	new Space(obj);
	
	dGeomID geom  = dCreateBox (0, 8,8,0.5); //geom
	Geom *data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	dGeomSetPosition (geom, x, y, z);

	//Graphics
	data->model = model[0];


	//flipper sensor
	dGeomID geom2 = dCreateBox (0, 3,3,2);
	data = new Geom(geom2, obj);
	data->surface.mu = 1.0;
	data->surface.spring = 0.0; //0 spring constanct = no collision forces
	dGeomSetPosition (geom2, x, y, z+0.76);

	data->flipper_geom = geom; //tmp debug solution
	//enable script execution on sensor triggering (but not when untriggered)
	data->Set_Sensor_Event ( ((Script*)1337) , NULL); //(triggered,untriggered)

	//
	}
	//
	else if (NH4)
	{
	Log_Add(2, "(hard-coded \"molecule\")");
	//
	//
	//

	Object *obj = new Object();
	new Space(obj);

	//center sphere
	dGeomID geom  = dCreateSphere (0, 1); //geom
	Geom *data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	data->Set_Buffer_Event(100000, 10000, (Script*)1337);

	dBodyID body1 = dBodyCreate (simulation_thread.world);

	dMass m;
	dMassSetSphereTotal (&m,60,1); //mass and radius
	dBodySetMass (body1, &m);

	new Body (body1, obj);

	dGeomSetBody (geom, body1);

	dBodySetPosition (body1, x, y, z);

	//Next, Graphics
	data->model = model[0];

	dReal pos[4][3] = {
		{0, 0, 1.052},

		{0, 1, -0.326},
		{0.946,  -0.326, -0.326},
		{-0.946, -0.326, -0.326}};

	dJointID joint;
	dBodyID body;

	int i;
	for (i=0; i<4; ++i) {
	//connected spheres
	geom  = dCreateSphere (0, 0.8); //geom
	data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	data->Set_Buffer_Event(100000, 10000, (Script*)1337);
	body = dBodyCreate (simulation_thread.world);

	dMassSetSphereTotal (&m,30,0.5); //mass and radius
	dBodySetMass (body, &m);

	new Body (body, obj);

	dGeomSetBody (geom, body);

	dBodySetPosition (body, x+pos[i][0], y+pos[i][1], z+pos[i][2]);

	//Next, Graphics
	data->model = model[1];

	//connect to main sphere
	
	joint = dJointCreateBall (simulation_thread.world, 0);

	Joint *jd = new Joint(joint, obj);
	jd->Set_Buffer_Event(1000, 50000, (Script*)1337);

	dJointAttach (joint, body1, body);
	dJointSetBallAnchor (joint, x+pos[i][0], y+pos[i][1], z+pos[i][2]);
	}
	//done
	//
	//
	}
	else if (sphere)
	{
	Log_Add(2, "(beachball)");
	//
	//
	//

	Object *obj = new Object();

	//center sphere
	dGeomID geom  = dCreateSphere (0, 1); //geom
	Geom *data = new Geom(geom, obj);
	data->surface.mu = 1.0;
	data->Set_Buffer_Event(1000, 1500, (Script*)1337);
	dBodyID body1 = dBodyCreate (simulation_thread.world);

	dMass m;
	dMassSetSphereTotal (&m,20,1); //mass+radius
	dBodySetMass (body1, &m);

	Body *b = new Body (body1, obj);

	dGeomSetBody (geom, body1);

	dBodySetPosition (body1, x, y, z);

	//set low drag
	b->Set_Linear_Drag(1);
	b->Set_Angular_Drag(1);

	//set spring surface for beachball (should be bouncy)
	data->surface.spring = 5000.0; //springy (soft)
	data->surface.damping = 0.0; //no damping (air drag is enough)

	//Next, Graphics
	data->model=model[0];
	}
	//
	else if (building)
	{
	Log_Add(2, "(hard-coded building)");
	//
	//

	Object *obj = new Object();
	new Space(obj);
	dBodyID old_body[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
	dBodyID old_pillar[4] = {0,0,0,0};

	dBodyID body[4];

	int j;
	for (j=0; j<3; ++j)
	{
		int i;
		dBodyID body1[12], body2[9];
		for (i=0; i<12; ++i)
		{
			dGeomID geom  = dCreateBox (0, 4,0.4,2.4); //geom
			Geom *data = new Geom(geom, obj);
			data->surface.mu = 1.0;
			data->Set_Buffer_Event(100000, 100000, (Script*)1337);

			body1[i] = dBodyCreate (simulation_thread.world);
			dGeomSetBody (geom, body1[i]);

			dMass m;
			dMassSetBoxTotal (&m,400,4,0.4,2.7); //mass+sides
			dBodySetMass (body1[i], &m);

			new Body (body1[i], obj);

			data->model = model[2];
		}
		
		const dReal k = 1.5*4+0.4/2;

		dBodySetPosition (body1[0], x-4, y-k, z+(2.4/2));
		dBodySetPosition (body1[1], x,   y-k, z+(2.4/2));
		dBodySetPosition (body1[2], x+4, y-k, z+(2.4/2));

		dBodySetPosition (body1[6], x+4, y+k, z+(2.4/2));
		dBodySetPosition (body1[7], x,   y+k, z+(2.4/2));
		dBodySetPosition (body1[8], x-4, y+k, z+(2.4/2));

		dMatrix3 rot;
		dRFromAxisAndAngle (rot, 0,0,1, M_PI/2);
		for (i=3; i<6; ++i)
			dBodySetRotation (body1[i], rot);
		for (i=9; i<12; ++i)
			dBodySetRotation (body1[i], rot);

		dBodySetPosition (body1[3], x+k,  y-4, z+(2.4/2));
		dBodySetPosition (body1[4], x+k, y, z+(2.4/2));
		dBodySetPosition (body1[5], x+k, y+4, z+(2.4/2));

		dBodySetPosition (body1[9], x-k, y+4, z+(2.4/2));
		dBodySetPosition (body1[10], x-k, y, z+(2.4/2));
		dBodySetPosition (body1[11], x-k, y-4, z+(2.4/2));

		//connect wall blocks in height
		for (i=0; i<12; ++i)
		{
			debug_joint_fixed(body1[i], old_body[i], obj);
			//move these bodies to list of old bodies
			old_body[i] = body1[i];
		}

		//connect wall blocks in sideway
		for (i=0; i<11; ++i)
			debug_joint_fixed (body1[i], body1[i+1], obj);
		debug_joint_fixed (body1[0], body1[11], obj);

		//walls done, floor/ceiling
		for (i=0; i<9; ++i)
		{
			dGeomID geom  = dCreateBox (0, 4,4,0.2); //geom
			Geom *data = new Geom(geom, obj);
			data->surface.mu = 1.0;
			data->Set_Buffer_Event(100000, 100000, (Script*)1337);

			body2[i] = dBodyCreate (simulation_thread.world);
			dGeomSetBody (geom, body2[i]);

			dMass m;
			dMassSetBoxTotal (&m,400,4,4,0.2); //mass+sides
			dBodySetMass (body2[i], &m);

			new Body (body2[i], obj);

			data->model = model[1];
		}

		const dReal k2=2.4-0.2/2;

		dBodySetPosition (body2[0], x-4, y-4, z+k2);
		debug_joint_fixed(body2[0], body1[0], obj);
		debug_joint_fixed(body2[0], body1[11], obj);
		dBodySetPosition (body2[1], x,   y-4, z+k2);
		debug_joint_fixed(body2[1], body1[1], obj);
		dBodySetPosition (body2[2], x+4, y-4, z+k2);
		debug_joint_fixed(body2[2], body1[2], obj);
		debug_joint_fixed(body2[2], body1[3], obj);

		dBodySetPosition (body2[3], x-4, y, z+k2);
		debug_joint_fixed(body2[3], body1[10], obj);
		dBodySetPosition (body2[4], x,   y, z+k2);
		dBodySetPosition (body2[5], x+4, y, z+k2);
		debug_joint_fixed(body2[5], body1[4], obj);

		dBodySetPosition (body2[6], x-4, y+4, z+k2);
		debug_joint_fixed(body2[6], body1[9], obj);
		debug_joint_fixed(body2[6], body1[8], obj);
		dBodySetPosition (body2[7], x,   y+4, z+k2);
		debug_joint_fixed(body2[7], body1[7], obj);
		dBodySetPosition (body2[8], x+4, y+4, z+k2);
		debug_joint_fixed(body2[8], body1[6], obj);
		debug_joint_fixed(body2[8], body1[5], obj);

		//join floor blocks
		//1: horisontal
		debug_joint_fixed (body2[0], body2[1], obj);
		debug_joint_fixed (body2[1], body2[2], obj);
		debug_joint_fixed (body2[3], body2[4], obj);
		debug_joint_fixed (body2[4], body2[5], obj);
		debug_joint_fixed (body2[6], body2[7], obj);
		debug_joint_fixed (body2[7], body2[8], obj);
		//2: vertical
		debug_joint_fixed (body2[0], body2[3], obj);
		debug_joint_fixed (body2[3], body2[6], obj);
		debug_joint_fixed (body2[1], body2[4], obj);
		debug_joint_fixed (body2[4], body2[7], obj);
		debug_joint_fixed (body2[2], body2[5], obj);
		debug_joint_fixed (body2[5], body2[8], obj);
	
		//pillars
		dGeomID geom;
		Geom *data;
		for (i=0; i<4; ++i)
		{
			geom  = dCreateCapsule (0, 0.3,1.4); //geom
			data = new Geom(geom, obj);
			data->surface.mu = 1.0;
			data->Set_Buffer_Event(100000, 100000, (Script*)1337);
			body[i] = dBodyCreate (simulation_thread.world);
	
			dMass m;
			dMassSetCapsuleTotal (&m,400,3,1,0.5); //mass, direction (3=z-axis), radius and length
			dBodySetMass (body[i], &m);
	
			new Body (body[i], obj);

			dGeomSetBody (geom, body[i]);
	
			//Next, Graphics
			data->model = model[0];
		}

		dBodySetPosition (body[0], x+2, y+2, z+2.4/2);
		debug_joint_fixed(body[0], body2[8], obj);
		debug_joint_fixed(body[0], body2[7], obj);
		debug_joint_fixed(body[0], body2[5], obj);
		debug_joint_fixed(body[0], body2[4], obj);

		dBodySetPosition (body[1], x+2, y-2, z+2.4/2);
		debug_joint_fixed(body[1], body2[1], obj);
		debug_joint_fixed(body[1], body2[2], obj);
		debug_joint_fixed(body[1], body2[4], obj);
		debug_joint_fixed(body[1], body2[5], obj);

		dBodySetPosition (body[2], x-2, y+2, z+2.4/2);
		debug_joint_fixed(body[2], body2[7], obj);
		debug_joint_fixed(body[2], body2[6], obj);
		debug_joint_fixed(body[2], body2[4], obj);
		debug_joint_fixed(body[2], body2[3], obj);

		dBodySetPosition (body[3], x-2, y-2, z+2.4/2);
		debug_joint_fixed(body[3], body2[0], obj);
		debug_joint_fixed(body[3], body2[1], obj);
		debug_joint_fixed(body[3], body2[3], obj);
		debug_joint_fixed(body[3], body2[4], obj);

		for (i=0; i<4; ++i)
		{
			debug_joint_fixed(body[i], old_pillar[i], obj);
			old_pillar[i] = body[i];
		}

		z+=2.4;
	}
	//
	//
	}
	//
	//
	else if (pillar)
	{
		Log_Add(2, "(hard-coded pillar)");

		//just one geom in this object
		Geom *g = new Geom(dCreateBox(0, 2,2,5), new Object());
		g->surface.mu = 1.0;

		//position
		dGeomSetPosition(g->geom_id, x,y,(z+2.5));

		//render
		g->model = model[0];

		//identification
		g->TMP_pillar_geom = true;

		//destruction
		g->Set_Buffer_Event(200000, 100000, (Script*)1337);
		g->TMP_pillar_graphics = model[1];
	}
	//
	//
	else if (tetrahedron)
	{
		Log_Add(2, "(hard-coded tetrahedron)");

		Object *obj = new Object();

		Geom *g = geom[0]->Create_Mesh(obj);
		g->model = model[0];

		//properties:
		g->surface.mu = 1.0;
		g->Set_Buffer_Event(100000, 50000, (Script*)1337);

		dBodyID body = dBodyCreate (simulation_thread.world);
		dMass m;
		dMassSetTrimeshTotal (&m,10,g->geom_id); //built-in feature in ode!

		//
		//need to make sure the mass is at (0,0,0):

		//offset geom position the same way as mass:
		dGeomSetBody (g->geom_id, body);
		dGeomSetOffsetPosition (g->geom_id, -m.c[0], -m.c[1], -m.c[2]);

		dMassTranslate(&m, -m.c[0], -m.c[1], -m.c[2]);
		dBodySetMass (body, &m);
		//
		//

		Body *b = new Body(body, obj);

		dBodySetPosition(b->body_id, x,y,z);
	}
	//
	//
	else
		Log_Add(-1, "trying to create unidentified object?!");

}

