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

#include "../shared/object.hpp"

#include <ode/ode.h>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include "../shared/threads.hpp"

#include "../shared/racetime_data.hpp"
#include "../shared/trimesh.hpp"
#include "../shared/log.hpp"
#include "../shared/track.hpp"
#include "../shared/joint.hpp"
#include "../shared/geom.hpp"
#include "../shared/body.hpp"

//load data for spawning object (object data), hard-coded debug version
Object_Template *Object_Template::Load(const char *path)
{
	printlog(1, "Loading object: %s", path);

	//see if already loaded
	if (Object_Template *tmp=Racetime_Data::Find<Object_Template>(path))
	{
		return tmp;
	}

	//full path to script
	char script[strlen(path)+strlen("/object.lua")+1];
	strcpy(script, path);
	strcat(script, "/object.lua");

	//load file as chunk
	if (luaL_loadfile(lua_sim, script))
	{
		printlog(0, "ERROR: could not load script \"%s\": \"%s\"!",
				script, lua_tostring(lua_sim, -1));
		lua_pop(lua_sim, -1);
		return NULL;
	}

	//execute chunk and look for one return
	if (lua_pcall(lua_sim, 0, 1, 0))
	{
		printlog(0, "ERROR: \"%s\" while running \"%s\"!",
				lua_tostring(lua_sim, -1), script);
		lua_pop(lua_sim, -1);
		return NULL;
	}

	//if not returned ok
	if (!lua_isfunction(lua_sim, -1))
	{
		printlog(0, "ERROR: no spawning function returned by \"%s\"!", script);
		return NULL;
	}

	//great!
	Object_Template *obj = new Object_Template(path);
	obj->spawn_script = luaL_ref(lua_sim, LUA_REGISTRYINDEX);
	return obj;
}

//spawn
//TODO: rotation. move both pos+rot to arrays
Object *Object_Template::Spawn (dReal x, dReal y, dReal z)
{
	printlog(2, "Spawning object at: %f %f %f", x,y,z);
	dReal pos[3]={x,y,z};
	//default to no rotation....
	dReal rot[9]={	1.0, 0.0, 0.0,
			0.0, 1.0, 0.0,
			0.0, 0.0, 1.0	};

	Object *obj = new Object(pos, rot);

	lua_rawgeti(lua_sim, LUA_REGISTRYINDEX, spawn_script);

	if (obj->Run(0, 0))
	{
		printlog(0, "ERROR: \"%s\" while spawning object!",
				lua_tostring(lua_sim, -1));
		lua_pop(lua_sim, -1);
		delete obj;
		return NULL;
	}

	return obj;
}

