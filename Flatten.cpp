 /*
  * Unitex
  *
  * Copyright (C) 2001-2007 Universit� Paris-Est Marne-la-Vall�e <unitex@univ-mlv.fr>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Unicode.h"
#include "Copyright.h"
#include "Fst2.h"
#include "FlattenFst2.h"
#include "IOBuffer.h"
#include "Error.h"


void usage() {
u_printf("%S",COPYRIGHT);
u_printf("Usage: Flatten <fst2> <type> [depth]\n"
       "     <fst2> : compiled grammar to flatten;\n"
       "     <type> : this parameter specifies the type of the resulting grammar\n"
       "              The 2 possibles values are:\n"
       "              FST : if the grammar is not a finite-state one, the program\n"
       "                    makes a finite-state approximation of it. The resulting\n"
       "                    FST2 will contain only one graph.\n"
       "              RTN : the grammar will be flattened according to the depth limit.\n"
       "                    The resulting grammar may not be finite-state.\n"
       "     [depth] : maximum subgraph depth to be flattened. If this parameter is\n"
       "               not precised, the value 10 is taken by default.\n"
       "\n\n"
       "Flattens a FST2 grammar into a finite state transducer in the limit of\n"
       "a recursion depth. The grammar <fst2> is replaced by its flattened equivalent.\n"
       "If the flattening process is complete, the resulting grammar contains only one\n"
       "graph.\n");
}



int main(int argc, char **argv) {
/* Every Unitex program must start by this instruction,
 * in order to avoid display problems when called from
 * the graphical interface */
setBufferMode();

if ((argc<3) || (argc>4)) {
   usage();
   return 0;
}

int RTN;
if (!strcmp(argv[2],"RTN")) {
   RTN=1;
} else if (!strcmp(argv[2],"FST")) {
   RTN=0;
}  else {
   error("Invalid parameter: %s\n",argv[2]);
   return 1;
}
int depth=10;
if (argc==4) {
   if (1!=sscanf(argv[3],"%d",&depth) || (depth<1)) {
      error("Invalid depth parameter %s\n",argv[3]);
      return 1;
   }
}
u_printf("Loading %s...\n",argv[1]);
Fst2* origin=load_fst2(argv[1],1);
if (origin==NULL) {
   error("Cannot load %s\n",argv[1]);
   return 1;
}
char temp[FILENAME_MAX];
strcpy(temp,argv[1]);
strcat(temp,".tmp.fst2");
switch (flatten_fst2(origin,depth,temp,RTN)) {
   case EQUIVALENT_FST:
      u_printf("The resulting grammar is an equivalent finite-state transducer.\n");
      break;
   case APPROXIMATIVE_FST:
      u_printf("The resulting grammar is a finite-state approximation.\n");
      break;
   case EQUIVALENT_RTN:
      u_printf("The resulting grammar is an equivalent FST2 (RTN).\n");
      break;
   default:; // FIXME: What if the execution reaches here, just in case ?
}
free_Fst2(origin);
remove(argv[1]);
rename(temp,argv[1]);
return 0;
}
