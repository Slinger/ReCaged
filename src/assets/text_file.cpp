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

#include "text_file.hpp"
#include "common/log.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//NOTE: it might seem strange to use C text processing in C++ code,
//but it is simply more suitable in this case

Text_File::Text_File ()
{
	fp = NULL; //default until attempting opening
	word_count = 0; //no words read yet

	//allocate buffer anyway (even if not open), if reopening
	buffer_size = INITIAL_TEXT_FILE_BUFFER_SIZE;
	list_size = INITIAL_TEXT_FILE_LIST_SIZE;
	buffer = new char[buffer_size];
	words = new char*[list_size];
}

#include <unistd.h>
Text_File::~Text_File ()
{
	Close();

	delete[] buffer;
	delete[] words;
}

bool Text_File::Open (const char *file)
{
	//close old
	Close();

	//open
#ifndef _WIN32
	//proper version
	fp = fopen(file, "r");
#else
	//windows always expects carriage return before newline
	//(since we are processing unix-based text files, go binary)
	fp = fopen(file, "rb");
#endif

	//check success
	if (fp)
	{
		return true;
	}
	else
	{
		Log_Add(-1, "Text_File could not open file %s!", file);
		return false;
	}
}

void Text_File::Close()
{
	//make sure no old data is left
	if (fp)
	{
		fclose (fp);
		Clear_List();
	}
}

bool Text_File::Read_Line ()
{
	//first check if we got an open file...
	if (!fp)
	{
		Log_Add(0, "WARNING: Text_File tried reading without an open file!");
		return false;
	}

	//remove the old words
	Clear_List();

	//the following actions goes false if end of file
	if (!Seek_First())
		return false;
	if (!Line_To_Buffer())
		return false;
	if (!Buffer_To_List())
		return false;

	//ok
	return true;
}

bool Text_File::Seek_First()
{
	char c;
	while (true)
	{
		c = fgetc(fp);
		if (c == EOF)
			return false;

		else if (!isspace(c)) //not space
		{
			if (c == '#') //commented line, get next
			{
				Throw_Line();
				return Seek_First();
			}

			//else, we now have first word
			//(push back first char)
			ungetc (c, fp);
			return true;
		}
	}
}

bool Text_File::Line_To_Buffer()
{
	//make sure to reallocate buffer if too small
	int text_read=0; //no text already read

	while (true)
	{
		//this should always work, but just to be safe
		if (!(fgets( (&buffer[text_read]) , (buffer_size-text_read) , fp)))
			return false;

		text_read = strlen(buffer);

		//if EOF or read a newline, everything ok
		if (feof(fp) || buffer[text_read-1] == '\n')
			return true;
		
		//else: I guess the buffer was too small...
		Log_Add(2, "Text_File line buffer was too small, resizing");
		buffer_size += INITIAL_TEXT_FILE_BUFFER_SIZE;

		//copy old vyffer content
		char *oldbuffer=buffer;
		buffer = new char[buffer_size];
		strcpy(buffer, oldbuffer);
		delete[] oldbuffer;
	}
}

bool Text_File::Buffer_To_List()
{
	char *buffer_ptr = buffer; //position buffer pointer to start of buffer

	while (true)
	{
		//seek for char not space (if not end of buffer)
		while (*buffer_ptr != '\0' && isspace(*buffer_ptr))
			++buffer_ptr;

		//reached end (end of buffer, comment or newline)
		if (*buffer_ptr == '\0' || *buffer_ptr == '#' || *buffer_ptr == '\n')
			break;

		//
		//ok got start of new word
		//

		//there are two kinds of "words"
		if (*buffer_ptr == '\"') //quotation: "word" begins after " and ends at " (or \0)
		{
			//go one step more (don't want " in word)
			++buffer_ptr;

			//wery unusual error (line ends after quotation mark)
			if (*buffer_ptr == '\0')
			{
				Log_Add(0, "WARNING: Text_File line ended just after quotation mark (ignored)...");
				break;
			}

			//ok
			Append_To_List(buffer_ptr);

			//find next " or end of line
			while (*buffer_ptr!='\"' && *buffer_ptr!='\0')
				++buffer_ptr;

			if (*buffer_ptr=='\0') //end of line before end of quote
				Log_Add(0, "WARNING: Text_File reached end of line before end of quote (ignored)...");
			else
			{
				*buffer_ptr = '\0'; //make this end (instead of ")
				++buffer_ptr; //jump over this "local" end
			}
		}
		else //normal: word begins after space and ends at space (or \0)
		{
			Append_To_List(buffer_ptr);

			//look for end of word (space or end of buffer)
			++buffer_ptr;
			while (*buffer_ptr != '\0' && !isspace(*buffer_ptr))
				++buffer_ptr;

			//if word ended by end of buffer, do nothing. else
			if (*buffer_ptr != '\0')
			{
				*buffer_ptr = '\0'; //mark as end of word
				++buffer_ptr; //jump over local end
			}
		}
	}

	//in case no words were read from buffer
	if (word_count==0)
		return false;
	
	return true;
}

bool Text_File::Throw_Line ()
{
	char c;
	while (true)
	{
		c = fgetc(fp);
		if (c == EOF)
			return false;
		else if (c == '\n')
			return true;
	}
}

void Text_File::Append_To_List(char *word)
{
	++word_count;

	if (word_count > list_size)
	{
		Log_Add(2, "Text_File word list was too small, resizing");
		list_size+=INITIAL_TEXT_FILE_LIST_SIZE;

		char **oldwords=words;
		words = new char*[list_size];
		memcpy(words, oldwords, word_count*sizeof(char*));
		delete[] oldwords;
	}

	words[word_count-1] = word; //point to word in buffer
}

void Text_File::Clear_List()
{
	//this isn't necessary (since word count is set to 0)
	for (int i=0; i<word_count; ++i)
		words[i]=NULL;
	//

	word_count = 0;
}

