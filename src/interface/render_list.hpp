/*
 * ReCaged - a Free Software, Futuristic, Racing Game
 *
 * Copyright (C) 2009, 2010, 2011, 2015 Mats Wahlberg
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

#ifndef _ReCaged_RENDER_LISTS_H
#define _ReCaged_RENDER_LISTS_H

//currently just list for components (geoms+bodies)
#define INITIAL_RENDER_LIST_SIZE 150

#include <SDL/SDL_mutex.h>

//functions
void Render_List_Update(); //create pos/rot list
bool Render_List_Updated(); //check if new frame
void Render_List_Prepare(); //switch rendering buffer+set camera matrix
void Render_List_Render(); //render latest list
void Render_List_Clear_Interface(); //free buffers ("render" and maybe "switch")
void Render_List_Clear_Simulation(); //free buffers ("generate" and maybe "switch")

#endif
