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
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include "Error.h"
#include "AbstractFilePlugCallback.h"
#include "Af_stdio.h"



struct AbstractFileSpace {
	t_fileio_func_array func_array;
	void* privateSpacePtr;
} ;


struct List_AbstractFileSpace {
	AbstractFileSpace afs;
	List_AbstractFileSpace* next;
} ;


struct List_AbstractFileSpace* p_abstract_file_space_list;



UNITEX_FUNC int UNITEX_CALL AddAbstractFileSpace(const t_fileio_func_array* func_array,void* privateSpacePtr)
{
	struct List_AbstractFileSpace* new_item;
	new_item = (struct List_AbstractFileSpace*)malloc(sizeof(struct List_AbstractFileSpace));
	if (new_item == NULL)
		return 0;

	new_item->afs.func_array = *func_array;
	new_item->afs.privateSpacePtr = privateSpacePtr;
	new_item->next = NULL;

	if (p_abstract_file_space_list == NULL)
		p_abstract_file_space_list = new_item;
	else {
		struct List_AbstractFileSpace* tmp = p_abstract_file_space_list;
		while ((tmp->next) != NULL)
			tmp = tmp->next;
		tmp->next = p_abstract_file_space_list;
	}

	if ((new_item->afs.func_array.fnc_Init_FileSpace) != NULL)
		(*(new_item->afs.func_array.fnc_Init_FileSpace))(new_item->afs.privateSpacePtr);

	return 1;
}

UNITEX_FUNC int UNITEX_CALL RemoveAbstractFileSpace(const t_fileio_func_array* func_array,void* privateSpacePtr)
{
	struct List_AbstractFileSpace* tmp = p_abstract_file_space_list;
	struct List_AbstractFileSpace* tmp_previous = NULL;

	while (tmp != NULL)
	{
		t_fileio_func_array test_func_array = tmp->afs.func_array;
		t_fileio_func_array request_func_array = *func_array;
		//if ((test_func_array == request_func_array) && (tmp->afs.privateSpacePtr == privateSpacePtr))
		//if ((tmp->afs.func_array == (*func_array)) && (tmp->afs.privateSpacePtr == privateSpacePtr))

		if ((memcmp(&tmp->afs.func_array,func_array,sizeof(t_fileio_func_array))==0) &&
			(tmp->afs.privateSpacePtr == privateSpacePtr))
		{
			if (tmp_previous == NULL)
				p_abstract_file_space_list = tmp->next;
			else
				tmp_previous->next = tmp->next;

			if ((tmp->afs.func_array.fnc_Uninit_FileSpace) != NULL)
				(*(tmp->afs.func_array.fnc_Uninit_FileSpace))(tmp->afs.privateSpacePtr);

			free(tmp);
			return 1;
		}
		tmp_previous = tmp;
		tmp = tmp->next;
	}
	return 0;
}

const AbstractFileSpace * GetFileSpaceForFileName(const char*name)
{
	const struct List_AbstractFileSpace* tmp = p_abstract_file_space_list;

	while (tmp != NULL)
	{
		const AbstractFileSpace * test_afs = &(tmp->afs);
		if (tmp->afs.func_array.fnc_is_filename_object(name,tmp->afs.privateSpacePtr) != 0)
			return test_afs;		

		tmp = tmp->next;
	}
	return NULL;
}
/**********/
typedef struct _ABSTRACTFILE_REAL
{
	union
	{
		FILE* f;
		ABSTRACTFILE_PTR fabstr;
	} ;
	const AbstractFileSpace * afs;
} ABSTRACTFILE_REAL;



ABSTRACTFILE* af_fopen(const char* name,const char* MODE)
{
	ABSTRACTFILE_REAL* vf= (ABSTRACTFILE_REAL*)malloc(sizeof(ABSTRACTFILE_REAL));
	const AbstractFileSpace * pafs ;
	if (vf==NULL) {
		fatal_alloc_error("af_fopen");
		return NULL;
	}

	pafs = GetFileSpaceForFileName(name);
	vf->afs = pafs;
	if (pafs == NULL) {
		vf->f = fopen(name,MODE);
		if (vf->f == NULL) {
			free(vf);
			vf = NULL;
		}
	}
	else
	{
        TYPEOPEN_MF TypeOpen;
        if ((*(MODE))=='w')
            TypeOpen = OPEN_CREATE_MF;
        else
            TypeOpen = OPEN_READWRITE_MF;
        //vfRet -> Std_Stream_Type = STD_STREAM_MEMFILE;
		vf->afs = pafs;
		vf->fabstr = (*(pafs->func_array.fnc_memOpenLowLevel))(name, TypeOpen,
			                      0,0,pafs->privateSpacePtr);

        if (vf->fabstr == NULL)
        {
            free(vf);
            vf= NULL;
        }
        else
        {
          if ((*(MODE))=='a')
              (*(pafs->func_array.fnc_memLowLevelSeek))(vf->fabstr,0,SEEK_END,pafs->privateSpacePtr);        
        }
	}
	return (ABSTRACTFILE*)vf;
}


int af_fclose(ABSTRACTFILE* stream)
{
	ABSTRACTFILE_REAL* p_abfr=(ABSTRACTFILE_REAL*)stream;
	ABSTRACTFILE_REAL abfr=*p_abfr;
	free(p_abfr);
	if (abfr.afs == NULL)
		return fclose(abfr.f);
	else
		return (*(abfr.afs->func_array.fnc_memLowLevelClose))(abfr.fabstr,abfr.afs->privateSpacePtr);
}


size_t af_fread(void *ptr,size_t sizeItem,size_t nmemb,ABSTRACTFILE *stream)
{
	ABSTRACTFILE_REAL* p_abfr=(ABSTRACTFILE_REAL*)stream;
	if (p_abfr->afs == NULL)
		return fread(ptr,sizeItem,nmemb,p_abfr->f);
	else{
		size_t nbByteToRead = sizeItem * nmemb;
		size_t res = (*(p_abfr->afs->func_array.fnc_memLowLevelRead))(p_abfr->fabstr,ptr,nbByteToRead,p_abfr->afs->privateSpacePtr);
		if ((res > 0) && (sizeItem>0))
			res /= sizeItem;
		return res;
	}
}

size_t af_fwrite(const void *ptr,size_t sizeItem,size_t nmemb,ABSTRACTFILE *stream)
{
	ABSTRACTFILE_REAL* p_abfr=(ABSTRACTFILE_REAL*)stream;
	if (p_abfr->afs == NULL)
		return fwrite(ptr,sizeItem,nmemb,p_abfr->f);
	else {
		size_t nbByteToWrite = sizeItem * nmemb;
		size_t res = (*(p_abfr->afs->func_array.fnc_memLowLevelWrite))(p_abfr->fabstr,ptr,nbByteToWrite,p_abfr->afs->privateSpacePtr);
		if ((res > 0) && (sizeItem>0))
			res /= sizeItem;
		return res;
	}
}


char *af_fgets(char * _Buf, int count, ABSTRACTFILE * stream)
{
	ABSTRACTFILE_REAL* p_abfr=(ABSTRACTFILE_REAL*)stream;
	if (p_abfr->afs == NULL)
		return fgets(_Buf,count,p_abfr->f);
	else { 
		char* retval = _Buf;
		char* pointer = _Buf;

		if(retval!=NULL)
        {
            while (--count)
            {
              char ch;
              if (((*(p_abfr->afs->func_array.fnc_memLowLevelRead))(p_abfr->fabstr,&ch,1,p_abfr->afs->privateSpacePtr))!=1)
                    {
						if (pointer == _Buf)
							return NULL;
						else
							return retval;
                    }

              (*(pointer++)) = ch ;
              pointer++;
              if (ch == '\n')
                break;
            }
			*pointer = '\0';
        }
		return retval;	
	}
}


int af_fseek(ABSTRACTFILE* stream, long offset, int whence)
{
	ABSTRACTFILE_REAL* p_abfr=(ABSTRACTFILE_REAL*)stream;
	if (p_abfr->afs == NULL)
		return fseek(p_abfr->f,offset,whence);
	else
		return (*(p_abfr->afs->func_array.fnc_memLowLevelSeek))(p_abfr->fabstr, offset,whence,p_abfr->afs->privateSpacePtr);
}

long af_ftell(ABSTRACTFILE* stream)
{
	ABSTRACTFILE_REAL* p_abfr=(ABSTRACTFILE_REAL*)stream;
	if (p_abfr->afs == NULL)
		return ftell(p_abfr->f);
	else {
		afs_size_type pos=0;
        (*(p_abfr->afs->func_array.fnc_memLowLevelTell))(p_abfr->fabstr, &pos,p_abfr->afs->privateSpacePtr);
        return (long)pos;
	}
}


int af_feof(ABSTRACTFILE* stream)
{
	ABSTRACTFILE_REAL* p_abfr=(ABSTRACTFILE_REAL*)stream;
	if (p_abfr->afs == NULL)
		return feof(p_abfr->f);
	else {
		afs_size_type pos=0;
		afs_size_type sizeFile = 0;
        (*(p_abfr->afs->func_array.fnc_memLowLevelTell))(p_abfr->fabstr, &pos,p_abfr->afs->privateSpacePtr);
		(*(p_abfr->afs->func_array.fnc_memLowLevelGetSize))(p_abfr->fabstr, &sizeFile,p_abfr->afs->privateSpacePtr);
		if (sizeFile == pos)
			return 1;
		else
			return 0;
	}
}

int af_ungetc(int ch, ABSTRACTFILE* stream)
{
	ABSTRACTFILE_REAL* p_abfr=(ABSTRACTFILE_REAL*)stream;
	if (p_abfr->afs == NULL)
		return ungetc(ch,p_abfr->f);
	else
		return (af_fseek(stream,-1,SEEK_CUR) != 0 ) ? EOF:ch;
}

int af_rename(const char * OldFilename, const char * NewFilename)
{
	const AbstractFileSpace * pafs = GetFileSpaceForFileName(OldFilename);
	if (pafs==NULL)
		return rename(OldFilename,NewFilename);
	else
		return (*(pafs->func_array.fnc_memFileRename))(OldFilename,NewFilename,pafs->privateSpacePtr);
}

int af_remove(const char * Filename)
{
	const AbstractFileSpace * pafs = GetFileSpaceForFileName(Filename);
	if (pafs==NULL)
		return remove(Filename);
	else
		return (*(pafs->func_array.fnc_memFileRemove))(Filename,pafs->privateSpacePtr);
}

ABSTRACTFILE_REAL VF_StdIn = { {stdin},NULL };
ABSTRACTFILE_REAL VF_StdOut = { {stdout},NULL };
ABSTRACTFILE_REAL VF_StdErr = { {stderr},NULL };

ABSTRACTFILE* return_af_stdin()
{
	return (ABSTRACTFILE*)&VF_StdIn;
}

ABSTRACTFILE* return_af_stdout()
{
	return (ABSTRACTFILE*)&VF_StdOut;
}

ABSTRACTFILE* return_af_stderr()
{
	return (ABSTRACTFILE*)&VF_StdErr;
}

int IsStdIn(ABSTRACTFILE* stream)
{
	ABSTRACTFILE_REAL* p_abfr=(ABSTRACTFILE_REAL*)stream;
	return (((p_abfr->f)==stdin) && (p_abfr->afs==NULL));
}

int IsStdWrite(ABSTRACTFILE* stream,enum stdwrite_kind * p_std_write)
{
	ABSTRACTFILE_REAL* p_abfr=(ABSTRACTFILE_REAL*)stream;
	if (p_abfr->afs != NULL)
	  return 0;

	if (((p_abfr->f)==stdout))
	{
		if (p_std_write != NULL)
			*p_std_write = stdwrite_kind_out;
		return 1;
	}

	if (((p_abfr->f)==stderr))
	{
		if (p_std_write != NULL)
			*p_std_write = stdwrite_kind_err;
		return 1;
	}

	return 0;
}