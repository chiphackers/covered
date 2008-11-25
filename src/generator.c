/*
 Copyright (c) 2006-2008 Trevor Williams

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "generator.h"
#include "profiler.h"


extern db**         db_list;
extern unsigned int curr_db;


static void generator_output_instance_tree(
  funit_inst* root
) { PROFILE(GENERATOR_OUTPUT_INSTANCE_TREE);

  PROFILE_END;

}

void generator_output() { PROFILE(GENERATOR_OUTPUT);

  inst_link* instl = db_list[curr_db]->inst_head;

  while( instl != NULL ) {
    generator_output_instance_tree( instl->inst );
    instl = instl->next;
  }

  PROFILE_END;

}

/*
 $Log$
*/

