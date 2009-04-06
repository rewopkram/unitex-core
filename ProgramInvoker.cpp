 /*
  * Unitex
  *
  * Copyright (C) 2001-2009 Universit� Paris-Est Marne-la-Vall�e <unitex@univ-mlv.fr>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 2.1 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  * 
  * You should have received a copy of the GNU Lesser General Public
  * License along with this library; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
  *
  */

#include "ProgramInvoker.h"
#include "Error.h"
#include <string.h>

/**
 * Allocates, initializes and returns a new program invoker.
 */
ProgramInvoker* new_ProgramInvoker(MAIN_FUNCTION f,char* name) {
if (f==NULL) {
   fatal_error("NULL main pointer in new_ProgramInvoker\n");
}
ProgramInvoker* res=(ProgramInvoker*)malloc(sizeof(ProgramInvoker));
if (res==NULL) {
   fatal_alloc_error("new_ProgramInvoker");
}
res->main=f;
res->args=new_vector_ptr(16);
add_argument(res,name);
return res;
}


/**
 * Frees all the memory associated to the given invoker.
 */
void free_ProgramInvoker(ProgramInvoker* i) {
if (i==NULL) return;
free_vector_ptr(i->args,free);
free(i);
}


/**
 * Add the given argument to the given invoker.
 */
void add_argument(ProgramInvoker* invoker,char* arg) {
char* tmp=strdup(arg);
if (tmp==NULL) {
   fatal_alloc_error("add_argument");
}
vector_ptr_add(invoker->args,tmp);
}


/**
 * Invoke the main function.
 */
int invoke(ProgramInvoker* invoker) {
return invoker->main(invoker->args->nbelems,(char**)(invoker->args->tab));
}


