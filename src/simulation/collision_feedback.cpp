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

#include "collision_feedback.hpp"

Collision_Feedback *Collision_Feedback::head = NULL;

Collision_Feedback::Collision_Feedback(dJointID joint, Geom *g1, Geom *g2)
{
	geom1 = g1;
	geom2 = g2;

	//make sure initialized to 0 (in case joint doesn't return any data...)
	feedback.f1[0]=0;
	feedback.f1[1]=0;
	feedback.f1[2]=0;
	feedback.f2[0]=0;
	feedback.f2[1]=0;
	feedback.f2[2]=0;

	//set
	dJointSetFeedback(joint, &feedback);

	//add to list
	next = head;
	head = this;
}

void Collision_Feedback::Physics_Step(dReal step)
{
	Collision_Feedback *prev;
	dReal force1, force2;

	while (head)
	{
		//calculate length (absolute value) of each force
		force1 = dLENGTH(head->feedback.f1);
		force2 = dLENGTH(head->feedback.f2);

		//pass biggest force to both geoms
		if (force1 > force2)
		{
			head->geom1->Damage_Buffer(force1, step);
			head->geom2->Damage_Buffer(force1, step);
		}
		else //f2>f1
		{
			head->geom1->Damage_Buffer(force2, step);
			head->geom2->Damage_Buffer(force2, step);
		}

		//remove
		prev = head;
		head = head->next;
		delete prev;
	}
}

