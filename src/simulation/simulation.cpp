/*
 * ReCaged - a Free Software, Futuristic, Racing Simulator
 *
 * Copyright (C) 2009, 2010, 2011, 2012 Mats Wahlberg
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

#include <SDL/SDL_timer.h>
#include <SDL/SDL_mutex.h>
#include <ode/ode.h>

extern "C" {
#include HEADER_LUA_H
#include HEADER_LUALIB_H
#include HEADER_LAUXLIB_H
}

#include "../shared/threads.hpp"
#include "../shared/internal.hpp"
#include "../shared/track.hpp"
#include "../shared/log.hpp"
#include "../shared/body.hpp"
#include "../shared/geom.hpp"
#include "../shared/camera.hpp"
#include "../shared/car.hpp"
#include "../shared/joint.hpp"

#include "collision_feedback.hpp"
#include "event_buffers.hpp"
#include "timers.hpp"

#include "../interface/render_list.hpp"

//list of lua functions:
static int lua_simulation_start (lua_State *lua);
static int lua_simulation_stop (lua_State *lua);
static int lua_simulation_run (lua_State *lua);
static int lua_simulation_tmpload (lua_State *lua);

const luaL_Reg lua_simulation[] =
{
	{"start", lua_simulation_start},
	{"stop", lua_simulation_stop},
	{"run", lua_simulation_run},
	{"tmpload", lua_simulation_tmpload},
	{NULL, NULL}
};
//

//keep track of what to do
runlevel_type runlevel = paused; //TODO: might be private in future!

//
//TODO: move the following to rc lua script!!!
//

Uint32 starttime = 0;
Uint32 racetime = 0;
Uint32 simtime = 0; 
unsigned int simulation_lag = 0;
unsigned int simulation_count = 0;

bool Simulation_Init(void)
{
	Log_Add(0, "Initiating simulation");

	//lua state
	lua_sim = luaL_newstate();
	if (!lua_sim)
	{
		Log_Add(0, "ERROR: could not create new lua state");
		return false;
	}

	//lua libraries allowed/needed:
	luaopen_string(lua_sim);
	luaopen_table(lua_sim);
	luaopen_math(lua_sim);

	//custom libraries:
	luaL_register(lua_sim, "log", lua_log);

	//ode
	dInitODE2(0);
	dAllocateODEDataForThread(dAllocateFlagBasicData | dAllocateFlagCollisionData);

	world = dWorldCreate();

	//set global ode parameters (except those specific to track)
	space = dHashSpaceCreate(0);
	dHashSpaceSetLevels(space, internal.hash_levels[0], internal.hash_levels[1]);

	contactgroup = dJointGroupCreate(0);

	dWorldSetQuickStepNumIterations (world, internal.iterations);

	//autodisable
	dWorldSetAutoDisableFlag (world, 1);
	dWorldSetAutoDisableLinearThreshold (world, internal.dis_linear);
	dWorldSetAutoDisableAngularThreshold (world, internal.dis_angular);
	dWorldSetAutoDisableSteps (world, internal.dis_steps);
	dWorldSetAutoDisableTime (world, internal.dis_time);

	//default joint forces
	dWorldSetERP (world, internal.erp);
	dWorldSetCFM (world, internal.cfm);

	return true;
}


int Simulation_Loop (void *d)
{
	Log_Add(1, "Starting simulation loop");

	simtime = SDL_GetTicks(); //set simulated time to realtime
	Uint32 realtime; //real time (with possible delay since last update)
	Uint32 stepsize_ms = (Uint32) (internal.stepsize*1000.0+0.0001);
	dReal divided_stepsize = internal.stepsize/internal.multiplier;

	//keep running until done
	while (runlevel != done)
	{
		//only if in active mode do we simulate
		if (runlevel == running)
		{
			//technically, collision detection doesn't need this, but this is easier
			SDL_mutexP(ode_mutex);

			for (int i=0; i<internal.multiplier; ++i)
			{
				Car::Physics_Step(divided_stepsize); //control, antigrav...
				Body::Physics_Step(divided_stepsize); //drag (air/liquid "friction") and respawning
				camera.Physics_Step(divided_stepsize); //calculate velocity and move

				Geom::Clear_Collisions(); //set all collision flags to false

				//perform collision detection
				dSpaceCollide (space, (void*)(&divided_stepsize), &Geom::Collision_Callback);
				Wheel::Generate_Contacts(divided_stepsize); //add tyre contact points

				Geom::Physics_Step(); //sensor/radar handling

				dWorldQuickStep (world, divided_stepsize);
				dJointGroupEmpty (contactgroup);

				Joint::Physics_Step(divided_stepsize); //joint forces
				Collision_Feedback::Physics_Step(divided_stepsize); //forces from collisions
			}

			//previous simulations might have caused events (to be processed by scripts)...
			Event_Buffers_Process(internal.stepsize);

			//process timers:
			Animation_Timer::Events_Step(internal.stepsize);

			//done with ode
			SDL_mutexV(ode_mutex);

			//opdate for interface:
			Render_List_Update(); //make copy of position/rotation for rendering
		}
		else
			Render_List_Update(); //still copy (in case camera updates or something)

		//broadcast to wake up sleeping threads
		if (internal.sync_interface)
		{
			SDL_mutexP(sync_mutex);
			SDL_CondBroadcast (sync_cond);
			SDL_mutexV(sync_mutex);
		}

		simtime += stepsize_ms;

		//sync simulation with realtime
		if (internal.sync_simulation)
		{
			//get some time to while away?
			realtime = SDL_GetTicks();
			if (simtime > realtime)
			{
				//busy-waiting:
				if (internal.spinning)
					while (simtime > SDL_GetTicks());
				//sleep:
				else
					SDL_Delay (simtime - realtime);
			}
			else
				++simulation_lag;
		}

		//count how many steps
		++simulation_count;
	}

	//during simulation, memory might be allocated, remove
	Wheel::Clear_List();

	//remove buffers for building rendering list
	Render_List_Clear_Simulation();

	return 0;
}

//not lua
void Simulation_Quit (void)
{
	Log_Add(1, "Quit simulation");
	lua_close(lua_sim);
	dJointGroupDestroy (contactgroup);
	dSpaceDestroy (space);
	dWorldDestroy (world);
	dCloseODE();
}


//for lua start/stop thread:
static int lua_simulation_start (lua_State *lua)
{
	simulation_thread = SDL_CreateThread (Simulation_Loop, NULL);
	starttime = SDL_GetTicks();
	return 0;
}

static int lua_simulation_stop (lua_State *lua)
{
	runlevel = done;
	SDL_WaitThread (simulation_thread, NULL);
	simulation_thread=NULL;
	return 0;
}

static int lua_simulation_run (lua_State *lua)
{
	if (lua_gettop(lua)==1 && lua_isboolean(lua, 1))
	{
		bool run = lua_toboolean(lua, 1);
		if (run)
			runlevel = running;
		else
			runlevel = paused;

	}
	else
	{
		lua_pushstring(lua, "simulation.run: expects single boolean value");
		lua_error(lua);
	}

	return 0;
}


#include "../shared/profile.hpp"

static int lua_simulation_tmpload (lua_State *lua)
{
	Profile *prof = Profile_Load ("profiles/default");
	if (!prof)
		return 0; //GOTO: profile menu

	//TODO: probably Racetime_Data::Destroy_All() here
	if (!load_track("worlds/Sandbox/tracks/Box"))
		return 0; //GOTO: track selection menu

	//TMP: load some objects for online spawning
	if (	!(box = Object_Template::Load("objects/misc/box"))) //		||
		//!(sphere = Object_Template::Load("objects/misc/beachball"))||
		//!(funbox = Object_Template::Load("objects/misc/funbox"))	||
		//!(molecule = Object_Template::Load("objects/misc/NH4"))	)
		return 0;
	//

	Car_Template *car_template=Car_Template::Load("teams/Nemesis/cars/Venom");

	if (!car_template)
		return 0;

	Trimesh_3D *tyre = Trimesh_3D::Quick_Load_Conf("worlds/Sandbox/tyres/diameter/2/Slick", "tyre.conf");
	Trimesh_3D *rim = Trimesh_3D::Quick_Load_Conf("teams/Nemesis/rims/diameter/2/Split", "rim.conf");
	//good, spawn
	Car *car = car_template->Spawn(
		track.start[0], //x
		track.start[1], //y
		track.start[2], //z
		tyre, rim);

	//this single player/profile controls all cars for now... and ladt one by default
	prof->car = car;
	camera.Set_Car(car);

	return 0;
}

