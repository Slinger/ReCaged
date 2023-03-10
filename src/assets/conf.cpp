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

#include "conf.hpp"

#include "text_file.hpp"
#include "common/log.hpp"

#include <string.h>
#include <stdlib.h>

#include <string>

#include <ode/ode.h>

//loads configuration file to memory (using index)
bool Load_Conf (const char *name, char *memory, const struct Conf_Index index[])
{
	Log_Add(2, "Loading conf file: %s", name);

	Text_File file;
	if (!file.Open(name))
		return false;


	int i;
	int argnr;
	char *strleft; //text left in word if not completely converted

	//tmp loading variables:
	int tmpi;
	float tmpf;
	double tmpd;
	dReal tmpr;
	while (file.Read_Line())
	{
		//find matching index (loop until end of list or found matching
		for (i=0; ((index[i].type !=0) && (strcmp(index[i].name,file.words[0]) != 0) ); ++i);

		if (index[i].type==0) //not match, got to end
		{
			Log_Add(0, "WARNING: parameter \"%s\" does not match any parameter name!", file.words[0]);
			continue;
		}
		//else, we have a match

		//see if ammount of args is correct
		//argument name+values == words
		if (index[i].length+1 != file.word_count)
		{
			Log_Add(0, "WARNING: parameter \"%s\" has wrong ammount of args: expected: %i, got %i!",file.words[0], index[i].length, file.word_count-1);
			continue;
		}

		//loop through arguments
		for (argnr=0;argnr<index[i].length;++argnr)
		{
			//what type
			switch (index[i].type)
			{
				//float
				case 'f':
					tmpf = strtof(file.words[argnr+1], &strleft);

					if (strleft == file.words[argnr+1])
						Log_Add(0, "WARNING: \"%s\" is invalid for \"floating point\" parameter \"%s\"!", file.words[argnr+1], index[i].name);
					else //ok
						*( ((float*)(memory+index[i].offset))+argnr ) = tmpf;
				break;

				//double
				case 'd':
					tmpd = strtod(file.words[argnr+1], &strleft);

					if (strleft == file.words[argnr+1])
						Log_Add(0, "WARNING: \"%s\" is invalid for \"double precision floating point\" parameter \"%s\"!", file.words[argnr+1], index[i].name);
					else //ok
						*( ((double*)(memory+index[i].offset))+argnr ) = tmpd;
				break;

				//dReal
				case 'R':
					//there are two alternatives here (depending on how ode is configured): float or double
					//always read values as double, and then cast them to whatever "dReal" might be
					tmpr = strtod(file.words[argnr+1], &strleft);

					if (strleft == file.words[argnr+1])
						Log_Add(0, "WARNING: \"%s\" is invalid for \"dReal floating point\" parameter \"%s\"!", file.words[argnr+1], index[i].name);
					else
						*( ((dReal*)(memory+index[i].offset))+argnr ) = tmpr;
				break;

				//bool
				case 'b':
					//"true" if: "true", "on", "yes", "1"
					if (	(!strcasecmp(file.words[argnr+1], "true")) ||
						(!strcasecmp(file.words[argnr+1], "on")) ||
						(!strcasecmp(file.words[argnr+1], "yes")) ||
						(!strcmp(file.words[argnr+1], "1"))	)
						*(((bool*)(memory+index[i].offset))+argnr) = true;
					//false if: "false", "off", "no", "0"
					else if((!strcasecmp(file.words[argnr+1], "false")) ||
						(!strcasecmp(file.words[argnr+1], "off")) ||
						(!strcasecmp(file.words[argnr+1], "no")) ||
						(!strcmp(file.words[argnr+1], "0"))	)
						*(((bool*)(memory+index[i].offset))+argnr) = false;
					else //failure (unknown word)
						Log_Add(0, "WARNING: \"%s\" is invalid for \"boolean\" parameter \"%s\"!", file.words[argnr+1], index[i].name);
				break;

				//integer
				case 'i':
					tmpi = strtol (file.words[argnr+1], &strleft, 0);

					if (strleft == file.words[argnr+1])
						Log_Add(0, "WARNING: \"%s\" is invalid for \"integer\" parameter \"%s\"!", file.words[argnr+1], index[i].name);
					else
						*( ((int*)(memory+index[i].offset))+argnr ) = tmpi;
				break;

				//string
				case 's':
					//check length
					if (strlen(file.words[argnr+1]) >= Conf_String_Size) //equal or bigger than max size
						Log_Add(0, "WARNING: word in conf file was too big for direct string copy!");
					else //ok, just copy
						strcpy(*( ((Conf_String*)(memory+index[i].offset))+argnr ),    file.words[argnr+1] );
				break;

				//unknown
				default:
					Log_Add(0, "WARNING: parameter \"%s\" is of unknown type (\"%c\")!", file.words[0], index[i].type);
				break;
			}
		}

	}

	return true;
}
