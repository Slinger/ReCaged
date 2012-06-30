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

#ifndef _RC_RACETIME_DATA_H
#define _RC_RACETIME_DATA_H

#include <typeinfo>
#include <string.h>
#include "log.hpp"

//TODO: should be thread safe (no simultaneous loading of same data), support for
//tracking which thread got access to data or something.
class Racetime_Data
{
	public:
		static void Destroy_All();

		//find data that matches specified name and type
		//NOTE: actual function template declared in header, since each object that uses it must
		//instantiate needed function (this follows the "Borland model", which is supported by g++)
		template<typename T>
		static T *Find(const char *name)
		{
			Racetime_Data *tmp;
			T *casted;

			for (tmp=head; tmp; tmp=tmp->next) //loop
			{
				//type conversion+casting ok
				if ((!strcmp(tmp->name, name)) && (casted=dynamic_cast<T*>(tmp)))
				{
					printlog(1, "racetime data already existed for \"%s\" (already loaded)", name);
					return casted;
				}
			}

			return NULL; //else
		}

	protected:
		Racetime_Data(const char *name);
		//just make sure the subclass destructor gets called
		virtual ~Racetime_Data();

	private:
		char *name; //name of specific data

		static Racetime_Data *head;
		Racetime_Data *next;
};
#endif
