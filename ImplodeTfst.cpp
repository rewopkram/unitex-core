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

#include <stdio.h>
#include "Unicode.h"
#include "Copyright.h"
#include "IOBuffer.h"
#include "Tfst.h"
#include "String_hash.h"
#include "DELA.h"
#include "List_pointer.h"
#include "Error.h"
#include "Transitions.h"
#include "getopt.h"
#include "File.h"
#include "SingleGraph.h"


void implode(Tfst*,FILE*,FILE*);


void usage() {
u_printf("%S",COPYRIGHT);
u_printf("Usage: ImplodeTfst [OPTIONS] <tfst>\n"
         "\n"
         "  <tfst>: input text automaton file\n"
         "\n"
         "OPTIONS:\n"
         "  -o OUT/--output=OUT: resulting text automaton. By default, the input\n"
         "                       text automaton is modified.\n"
         "  -h/--help: this help\n"
         "\n"
         "Implodes the specified text automaton by merging together lexical entries\n"
         "which only differ in their inflectional features.\n");
}


int main(int argc, char ** argv) {
/* Every Unitex program must start by this instruction,
 * in order to avoid display problems when called from
 * the graphical interface */
setBufferMode();  

if (argc==1) {
   usage();
   return 0;
}

const char* optstring=":o:h";
const struct option lopts[]= {
      {"output",required_argument,NULL,'o'},
      {"help",no_argument,NULL,'h'},
      {NULL,no_argument,NULL,0}
};
int val,index=-1;
char input_tfst[FILENAME_MAX]="";
char input_tind[FILENAME_MAX]="";
char output_tfst[FILENAME_MAX]="";
char output_tind[FILENAME_MAX]="";
optind=1;
while (EOF!=(val=getopt_long(argc,argv,optstring,lopts,&index))) {
   switch(val) {
   case 'o': if (optarg[0]=='\0') {
                fatal_error("You must specify a non empty output file name: %s\n",optarg);
             }
             strcpy(output_tfst,optarg);
             break;
   case 'h': usage(); return 0;
   case ':': if (index==-1) fatal_error("Missing argument for option -%c\n",optopt); 
             else fatal_error("Missing argument for option --%s\n",lopts[index].name);
   case '?': if (index==-1) fatal_error("Invalid option -%c\n",optopt); 
             else fatal_error("Invalid option --%s\n",optarg);
             break;
   }
   index=-1;
}

if (optind!=argc-1) {
   fatal_error("Invalid arguments: rerun with --help\n");
}

strcpy(input_tfst,argv[optind]);
remove_extension(input_tfst,input_tind);
strcat(input_tind,".tind");

int no_explicit_output=0;
if (output_tfst[0]=='\0') {
   no_explicit_output=1;
   sprintf(output_tfst,"%s.new",input_tfst);
   sprintf(output_tind,"%s.new",input_tind);
}
u_printf("Loading '%s'...\n",input_tfst);
Tfst* tfst=open_text_automaton(input_tfst);
if (tfst==NULL) {
   fatal_error("Unable to load '%s'\n",input_tfst);
}
FILE* f_tfst=u_fopen(output_tfst,U_WRITE);
if (f_tfst==NULL) {
   fatal_error("Cannot open '%s' for writing\n",output_tfst);
}
FILE* f_tind=fopen(output_tind,"wb");
if (f_tind==NULL) {
   fatal_error("Cannot open '%s' for writing\n",output_tind);
}
implode(tfst,f_tfst,f_tind);
u_fclose(f_tfst);
fclose(f_tind);
close_text_automaton(tfst);

/* Finally, we rename files if we must modify the input text automaton */
if (no_explicit_output) {
   remove(input_tfst);
   remove(input_tind);
   rename(output_tfst,input_tfst);
   rename(output_tind,input_tind);
}
return 0;
}


/**
 * Returns  0 if a=b
 *         <0 if a<b
 *         >0 if a>b
 * Note that and b are supposed to be non NULL.
 */
int compare_for_implode(Transition* a,Transition* b) {
if (a->state_number==b->state_number) {
   return a->tag_number-b->tag_number;
}
return a->state_number-b->state_number;
}


/**
 * Inserts the given transition into the given transition list.
 * Removes duplicates.
 */
Transition* sorted_insert_for_implode(Transition* list,Transition* element) {
if (list==NULL) {
   element->next=NULL;
   return element;
}
int res=compare_for_implode(list,element);
if (res==0) {
   free_Transition_list(element);
   return list;
}
if (res>0) {
   /* If we must insert at the beginning of the list */
   element->next=list;
   return element;
}
list->next=sorted_insert_for_implode(list->next,element);
return list;
}


/**
 * Sorts the given transition list in increasing order
 * 1) by destination state and 2) by tag number.
 */
Transition* sort_for_implode(Transition* list) {
Transition* result=NULL;
while (list!=NULL) {
   Transition* tmp=list;
   list=list->next;
   result=sorted_insert_for_implode(result,tmp);
}
return result;
}


/**
 * This structure is used to represent a pair (imploded tag,original tag).
 * This is used to check that we do not merge tags if their text positions differ.
 */
struct implode_infos {
   struct dela_entry* entry;
   TfstTag* tag;
};


/**
 * Allocates, initializes and returns a struct implode_infos*
 */
struct implode_infos* new_implode_infos(struct dela_entry* entry,TfstTag* tag) {
struct implode_infos* res=(struct implode_infos*)malloc(sizeof(struct implode_infos));
if (res==NULL) {
   fatal_error("Not enough memory in new_implode_infos\n");
}
res->entry=entry;
res->tag=tag;
return res;
}


/**
 * Frees the structure, but not its fields.
 */
void free_implode_infos(struct implode_infos* infos) {
free(infos);
}


/**
 * Returns 1 if t1 and t2 have the same offsets; 0 otherwise.
 */
int compatible_offets(TfstTag* t1,TfstTag* t2) {
return t1->start_pos_token==t2->start_pos_token
    && t1->start_pos_char==t2->start_pos_char
    && t1->end_pos_token==t2->end_pos_token
    && t1->end_pos_char==t2->end_pos_char;
}


/**
 * This function takes a list of DELA entries and an entry to merge with them.
 */
struct list_pointer* merge_entry(struct list_pointer* list,struct dela_entry* entry,TfstTag* tag) {
if (list==NULL) {
   return new_list_pointer(new_implode_infos(entry,tag),NULL);
}
struct implode_infos* infos=(struct implode_infos*)(list->pointer);
struct dela_entry* e=infos->entry;
/* If we can merge the entry with the current list element */
if (!u_strcmp(e->inflected,entry->inflected) && !u_strcmp(e->lemma,entry->lemma)
    && same_semantic_codes(e,entry) && compatible_offets(infos->tag,tag)) {
   merge_inflectional_codes(e,entry);
   /* We can free the entry */
   free_dela_entry(entry);
   return list;
}
list->next=merge_entry(list->next,entry,tag);
return list;
}




/**
 * This function takes a tfst representing a text automaton
 * and implodes its tags, i.e. it merges transitions that point
 * on the same state and whose tags share the same semantic
 * codes. For instance, if there are two transition from X to Y
 * that are tagged with "{rouge,.A:fs}" and "{rouge,.A:ms}", then
 * we will merge them into the single one "{rouge,.A:fs:ms}".
 * The resulting tfst is stored into the given .tfst and .tind files.
 */
void implode(Tfst* tfst,FILE* f_tfst,FILE* f_tind) {
u_printf("Imploding tags...\n");
unichar tag_content[4096];
unichar tmp[4096];
u_fprintf(f_tfst,"%010d\n",tfst->N);
for (int i=1;i<=tfst->N;i++) {
   struct string_hash* new_tags=new_string_hash();
   /* We set the <E> tag as the #0 one */
   u_strcpy(tmp,"@<E>\n.\n");
   get_value_index(tmp,new_tags);
   if (i%100==0) {
      u_printf("Sentence %d/%d              \r",i,tfst->N);
   }
   load_sentence(tfst,i);
   //u_fprintf(output,"-%d %S\n",i,fst2->graph_names[i]);
   for (int j=0;j<tfst->automaton->number_of_states;j++) {
      /* For each state state of each automaton, we try to
       * implode the tags of the transition list */
      SingleGraphState state=tfst->automaton->states[j];
      //u_fprintf(output,"%C ",is_final_state(state)?'t':':');
      state->outgoing_transitions=sort_for_implode(state->outgoing_transitions);
      int previous_state=-1;
      struct list_pointer* entry_list=NULL;
      Transition* new_transitions=NULL;
      for (Transition* t=state->outgoing_transitions;;t=t->next) {
         if (previous_state!=-1 && (t==NULL || t->state_number!=previous_state)) {
            /* If we have finished to deal with a set of transitions
             * pointing on the same state number, then we have to save
             * the corresponding transition list */
            while (entry_list!=NULL) {
               struct implode_infos* infos=(struct implode_infos*)entry_list->pointer;
               struct dela_entry* entry=infos->entry;
               build_tag(entry,NULL,tag_content);
               free_dela_entry(entry);
               /* We use an existing tag already parameter with correct offsets */ 
               TfstTag* tag=infos->tag;
               unichar* old_content=tag->content;
               tag->content=tag_content;
               TfstTag_to_string(tag,tmp);
               tag->content=old_content;
               int tag_number=get_value_index(tmp,new_tags);
               new_transitions=new_Transition(tag_number,previous_state,new_transitions);
               struct list_pointer* ptr=entry_list;
               entry_list=entry_list->next;
               free(ptr);
            }
         }
         if (t==NULL) break;
         previous_state=t->state_number;
         TfstTag* tag=(TfstTag*)(tfst->tags->tab[t->tag_number]);
         unichar* input=tag->content;
         if (input[0]=='{' && input[1]!='\0') {
            /* We don't test {S} or {STOP} since we are supposed
             * to manipulate a text automaton that should not
             * contain such things */
            struct dela_entry* entry=tokenize_tag_token(input);
            entry_list=merge_entry(entry_list,entry,tag);
         } else {
            /* If we have a tag that is not a DELA entry, we just add
             * the corresponding transition */
            TfstTag_to_string(tag,tmp);
            int tag_number=get_value_index(tmp,new_tags);
            new_transitions=new_Transition(tag_number,previous_state,new_transitions);
         }
      }
      /* At the end, we replace the original transition list by the new one */
      free_Transition_list(state->outgoing_transitions);
      state->outgoing_transitions=new_transitions;
   }
   /* Finally, we save the current sentence */
   save_current_sentence(tfst,f_tfst,f_tind,new_tags->value,new_tags->size);
   /* We free the tags */
   free_string_hash(new_tags);
}
u_printf("Done.                          \n");
}
