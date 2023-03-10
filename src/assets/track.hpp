/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2014, 2015, 2015 Mats Wahlberg
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

#ifndef _ReCaged_TRACK_H
#define _ReCaged_TRACK_H
#include "assets/conf.hpp"
#include "object.hpp"
#include <GL/glew.h>
#include <ode/ode.h>

//Allocated at start
//(in contrary to the other structs, this is actually not allocated on runtime!)
//TODO: all this will be moved to Thread and new rendering System (with lua)
extern struct Track_Struct {
	//placeholder for stuff like if it's raining/snowing and lightsources
	float background[4];

	float clipping[2];
	int fog_mode;
	float fog_range[2];
	float fog_density;
	float fog_colour[4];

	float position[4]; //light position
	float ambient[4];
	float diffuse[4];
	float specular[4];
	
	dReal gravity[3];

	dReal density; //for air drag (friction)
	dReal wind[3];

	dReal start[3];
	float cam_start[3];
	float focus_start[3];

	dReal restart;

	Object *object;
	Space *space;
} track;
//index:

const struct Track_Struct track_defaults = {
	{0.53,0.81,0.92 ,1.0},
	{1.0, 1500.0},
	3,
	{1000.0, 1500.0},
	0.0015,
	{0.53,0.81,0.92 ,1.0},
	{-1.0,0.5,1.0,0.0},
	{0.0,0.0,0.0 ,1.0},
	{1.0,1.0,1.0 ,1.0},
	{1.0,1.0,1.0 ,1.0},
	{0, 0, -9.82},
	1.29,
	{0.0,0.0,0.0},
	{0,-50,1.5},
	{50,-100,5},
	{0,0,0},
	-80.0};

const struct Conf_Index track_index[] = {
	{"background",	'f',3,	offsetof(Track_Struct, background)},
	{"clipping",	'f',2,	offsetof(Track_Struct, clipping)},
	{"fog:mode",	'i',1,	offsetof(Track_Struct, fog_mode)},
	{"fog:range",	'f',2,	offsetof(Track_Struct, fog_range)},
	{"fog:density",	'f',1,	offsetof(Track_Struct, fog_density)},
	{"fog:colour",	'f',3,	offsetof(Track_Struct, fog_colour)},
	{"ambient",	'f',3,	offsetof(Track_Struct, ambient[0])},
	{"diffuse",	'f',3,	offsetof(Track_Struct, diffuse[0])},
	{"specular",	'f',3,	offsetof(Track_Struct, specular[0])},
	{"position",	'f',4,	offsetof(Track_Struct, position[0])},
	{"gravity",	'R',3,	offsetof(Track_Struct, gravity)},
	{"density",	'R',1,	offsetof(Track_Struct, density)},
	{"wind",	'R',3,	offsetof(Track_Struct, wind)},
	{"start",	'R',3,	offsetof(Track_Struct, start)},
	{"cam_start",	'f',3,	offsetof(Track_Struct, cam_start)},
	{"focus_start",	'f',3,	offsetof(Track_Struct, focus_start)},
	{"restart",	'R',1,	offsetof(Track_Struct, restart)},
	{"",0,0}};//end

bool load_track (const char *path);
void Track_Physics_Step();

#endif
