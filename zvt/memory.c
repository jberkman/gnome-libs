/*  memory.c - Zed's Virtual Terminal
 *  Copyright (C) 1998  Michael Zucchi
 *
 *  'quick' memory functions.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
  memory handling routines

  provides for very fast allocation of a 'rotating' memory pool

*/

#include <stdlib.h>
#include "lists.h"

#define d(x)

#define MEM_BLOCK_SIZE 10240

struct _mem {
  struct _mem *next;
  struct _mem *prev;
  int ptr;			/* where in the mem block are we up to? */
  char mem[MEM_BLOCK_SIZE];
};

/* setup the memory list */
int vt_mem_init(struct vt_list *mem_list)
{
  d(printf("mem_init called (%s)\n", __FILE__));
  vt_list_new(mem_list);
  return 0;
}

/* get a memory block from the current pool */
void *vt_mem_get(struct vt_list *mem_list, int size)
{
  char *out;
  struct _mem *block;
  size = (size+3) & (~3);	/* long align size */

  block = (struct _mem *) mem_list->tailpred;
  if (vt_list_empty(mem_list) || ((MEM_BLOCK_SIZE - block->ptr) < size)) {
    d(printf("out of room - allocating a new block\n"));
    /* must allocate a new memory block */
    block = malloc(sizeof(*block));
    block->ptr=0;
    /*current = block;*/
    vt_list_addtail(mem_list, (struct vt_listnode *)block);
    d(printf("block = %08x\n", block));
  }
  out = &block->mem[block->ptr];
  block->ptr += size;
  return out;
}

/* 'free' a memory block, from a given pool
   if it is the last in the pool, then free the 'pool'

   NOTE!!!
   This only works if memory is released in the same
   order it was allocated ... (and all intermediate
   memory segments must also be free'd)
*/
void vt_mem_push(struct vt_list *mem_list, void *mem, int size)
{
  struct _mem *wn, *nn;
  char *mem_ptr;

  size = (size+3) & (~3);	/* long align data */

  mem_ptr = mem;
  wn = (struct _mem *)mem_list->head;
  nn = wn->next;
  /* watchful readers will note that this scan is a little
     dangerous - if it doesn't find it in the first block,
     it better not find it in the next one (and free it and
     break the list */
  while (nn) {
    d(printf("scanning block %08x for a match\n", wn));
    if ((mem_ptr >= wn->mem) && (mem_ptr < wn->mem+MEM_BLOCK_SIZE)) {
      break;
    }
    wn = nn;
    nn = nn->next;
  }

  if (nn) {
    d(printf("match found!\n"));
    if ((mem_ptr+size) == &wn->mem[wn->ptr]) {
      d(printf("memory removed!\n"));
      vt_list_remove((struct vt_listnode *)wn);
      free(wn);
    }
  }
}

