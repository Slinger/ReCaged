/*
 * ReCaged - a Free Software, Futuristic, Racing Game
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

#ifndef _ReCaged_SCRIPT_H
#define _ReCaged_SCRIPT_H

#include "assets.hpp"

//script: human readable (read: not _programming_) langue which will
//describe what should be done when creating an object (components, joints...),
//and when an component is colliding ("sensor triggering", destroying and so on)
//function arguments can point at 3d files and other scripts and so on...
//
//(currently not used)
//
//>Allocated at start
class Script: public Assets
{
	public:
		Script(const char*);
		~Script();
};

#endif
