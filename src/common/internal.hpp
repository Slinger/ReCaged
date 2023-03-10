/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2014, 2015 Mats Wahlberg
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

#ifndef _ReCaged_INTERNAL_H
#define _ReCaged_INTERNAL_H
#include "assets/conf.hpp"
#include <stddef.h>
#include <ode/ode.h>

//important system configuration variables
extern struct internal_struct {
	//log settings
	int verbosity; //for stdout
	bool logfile;

	//for multithreading
	bool sync_simulation, sync_interface;
	bool spinning;

	//physics
	dReal stepsize;
	int iterations;
	int multiplier;
	int contact_points;
	dReal surface_layer;
	dReal erp,cfm;
	dReal linear_drag, angular_drag;

	dReal dis_linear, dis_angular, dis_time;
	int dis_steps;

	int hash_levels[2];

	bool temporal_coherence;

	//graphics
	int res[2]; //resolution
	bool culling;
	bool fullscreen;
	int msaa;
	int filter;
	bool separate_specular;
	float vfov, hfov;
	float dist;
} internal;

const struct internal_struct internal_defaults = {
	1, //verbosity 1 until changed
	true,
	true,true,
	false,
	0.01,
	5,
	4,
	20,
	0.001,
	0.8, 0.00001,
	5.0,5.0,
	0.05,0.10,0.5,
	1,
	{-1,4},
	true,
	//graphics
	{1280,720},
	true,
	false,
	4,
	1,
	true,
	75.0,
	0.0,
	2500.0};

const struct Conf_Index internal_index[] = {
	{"verbosity",		'i',1, offsetof(struct internal_struct, verbosity)},
	{"logfile",		'b',1, offsetof(struct internal_struct, logfile)},

	{"sync_simulation",	'b',1, offsetof(struct internal_struct, sync_simulation)},
	{"sync_interface",	'b',1, offsetof(struct internal_struct, sync_interface)},
	{"spinning",		'b',1, offsetof(struct internal_struct, spinning)},

	//physics
	{"stepsize",		'R',1, offsetof(struct internal_struct, stepsize)},
	{"iterations",		'i',1, offsetof(struct internal_struct, iterations)},
	{"multiplier",		'i',1, offsetof(struct internal_struct, multiplier)},
	{"contact_points",	'i',1, offsetof(struct internal_struct, contact_points)},
	{"surface_layer",	'R',1, offsetof(struct internal_struct, surface_layer)},
	{"default_erp",		'R',1, offsetof(struct internal_struct, erp)},
	{"default_cfm",		'R',1, offsetof(struct internal_struct, cfm)},
	{"default_linear_drag",	'R',1, offsetof(struct internal_struct, linear_drag)},
	{"default_angular_drag",'R',1, offsetof(struct internal_struct, angular_drag)},
	{"auto_disable_linear",	'R',1, offsetof(struct internal_struct, dis_linear)},
	{"auto_disable_angular",'R',1, offsetof(struct internal_struct, dis_angular)},
	{"auto_disable_time",	'R',1, offsetof(struct internal_struct, dis_time)},
	{"auto_disable_steps",	'i',1, offsetof(struct internal_struct, dis_steps)},
	{"hash_levels",		'i',2, offsetof(struct internal_struct, hash_levels)},
	{"temporal_coherence",	'b',1, offsetof(struct internal_struct, temporal_coherence)},

	//graphics
	{"resolution",		'i',2, offsetof(struct internal_struct, res)},
	{"backface_culling",	'b',1, offsetof(struct internal_struct, culling)},
	{"fullscreen",		'b',1, offsetof(struct internal_struct, fullscreen)},
	{"multisample",		'i',1, offsetof(struct internal_struct, msaa)},
	{"texture:filter",	'i',1, offsetof(struct internal_struct, filter)},
	{"texture:separate_specular",'b',1, offsetof(struct internal_struct, separate_specular)},
	{"vFOV",		'f',1, offsetof(struct internal_struct, vfov)},
	{"hFOV",		'f',1, offsetof(struct internal_struct, hfov)},
	{"eye_distance",	'f',1, offsetof(struct internal_struct, dist)},

	{"",0,0}};


#endif
