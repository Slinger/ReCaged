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

#include "profile.hpp"
#include "common/log.hpp"
#include "common/directories.hpp"
#include "text_file.hpp"
#include "conf.hpp"

Profile *profile_head = NULL;

void Profile_Remove (Profile *target)
{
	Log_Add(2, "removing profile");

	//remove from list
	if (!target->prev) //head
		profile_head = target->next;
	else //not head
		target->prev->next = target->next;

	if (target->next) //not last
		target->next->prev = target->prev;

	//remove profile
	free (target);
}

void Profile_Remove_All()
{
	while (profile_head)
		Profile_Remove(profile_head);
}


//inputs (loaded from keys.lst)
//0: forward
//1: reverse
//2: right
//3: left
//4: drifting brakes
//5: select camera 1
//6: select camera 2
//7: select camera 3
//8: select camera 4
const char *profile_input_list[] = { "forward", "reverse", "right", "left",
	"drift_brake", "camera1", "camera2", "camera3", "camera4" };


// required to iterate through an enum in C++
template <class Enum>
Enum & enum_increment(Enum & value, Enum begin, Enum end)
{
	return value = (value == end) ? begin : Enum(value + 1);
}

SDLKey & operator++ (SDLKey & key)
{
	return enum_increment(key, SDLK_FIRST, SDLK_LAST);
}

//translate button name to key number
SDLKey get_key (char *name)
{
	SDLKey key;

	for (key=SDLK_FIRST; key<SDLK_LAST; ++key)
		if (strcmp(SDL_GetKeyName(key), name) == 0)
			return key;

	//we only get here if no match found
	Log_Add(-1, "Key name %s didn't match any known key!", name);
	return SDLK_UNKNOWN;
}

//load profile (conf and key list)
Profile *Profile_Load (const char *path)
{
	Log_Add(1, "Loading profile: %s", path);

	//create
	Profile *prof = new Profile; //allocate
	prof->next = profile_head;
	prof->prev = NULL;
	profile_head = prof;
	if (prof->next)
		prof->next->prev=prof;

	*prof = profile_defaults; //set all to defaults

	//for finding
	Directories dirs;

	//load personal conf
	char conf[strlen(path)+13+1];//+1 for \0
	strcpy (conf,path);
	strcat (conf,"/profile.conf");

	if (!(dirs.Find(conf, CONFIG, READ) && Load_Conf(dirs.Path(), (char *)prof, profile_index)))
		Log_Add(0, "WARNING: no conf file for profile, falling back to defaults");

	//set camera
	if (!(prof->camera_default >0 && prof->camera_default <5))
	{
		Log_Add(-1, "Default camera should be a value between 1 and 4!");
		prof->camera_default=3;
	}

	//load key list
	char list[strlen(path)+9+1];
	strcpy (list,path);
	strcat (list,"/keys.lst");

	Log_Add(2, "Loading key list: %s", list);
	Text_File file;

	if (dirs.Find(list, CONFIG, READ) && file.Open(dirs.Path()))
	{
		while (file.Read_Line())
		{

			//find match
			for (int i=0; i<9; ++i)
			{
				//match!
				if (!strcmp(profile_input_list[i], file.words[0]))
				{
					//these are rather crude, but doesn't matter:
					//they're safe and will be replaced when moving to lua
					if (file.word_count == 3 && !strcmp(file.words[1], "key")) //keyboard
					{
						prof->input[i].key = get_key(file.words[2]);
						if (prof->input[i].key == SDLK_UNKNOWN)
							Log_Add(-1, "Ignoring invalid keyboard button \"%s\"", file.words[2]);
					}
					else if (file.word_count == 5 && !strcmp(file.words[1], "axis")) //joystick axis
					{
						prof->input[i].axis = atoi (file.words[2]);
						prof->input[i].axis_min = atoi (file.words[3]);
						prof->input[i].axis_max = atoi (file.words[4]);
					}
					else if (file.word_count == 3 && !strcmp(file.words[1], "button")) //joystick button
					{
						prof->input[i].button = atoi (file.words[2]);
					}
					else if (file.word_count == 4 && !strcmp(file.words[1], "hat")) //joystick hat
					{
						prof->input[i].hat = atoi (file.words[2]);

						if (!strcmp(file.words[3], "up"))
							prof->input[i].hatpos = SDL_HAT_UP;
						else if (!strcmp(file.words[3], "down"))
							prof->input[i].hatpos = SDL_HAT_DOWN;
						else if (!strcmp(file.words[3], "right"))
							prof->input[i].hatpos = SDL_HAT_RIGHT;
						else if (!strcmp(file.words[3], "left"))
							prof->input[i].hatpos = SDL_HAT_LEFT;
						else
						{
							prof->input[i].hat=255;
							Log_Add(-1, "Ignoring invalid joystick hat direction \"%s\"", file.words[2]);
						}
					}
					else
						Log_Add(-1, "Invalid/malformed input action \"%s\"", file.words[0]);
				}
			}
		}
	}
	else
		Log_Add(0, "WARNING: no key mapping list for profile, falling back to defaults");

	return prof;
}

