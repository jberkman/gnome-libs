/*  memory.c - Zed's Virtual Terminal
 *  Copyright (C) 1998  Michael Zucchi
 *
 *  'quick' memory functions.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
  memory handling routines

  provides for very fast allocation of a 'rotating' memory pool

*/

#include <stdlib.h>
#include "lists.h"
#include "memory.h"

#define d(x)

#define MEM_BLOCK_SIZE 10240
#define MEM_BLOCK_ALIGN 0x03	/* align mask 3=4 bytes, 7=8 bytes */

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
  size = (size+MEM_BLOCK_ALIGN) & (~MEM_BLOCK_ALIGN);	/* long align size */

  d(printf("mem_get(%d)\n", size));
  d(printf("  ptr = %d\n", vt_list_empty(mem_list)?0:((struct _mem *)mem_list->tailpred)->ptr));

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
  d(printf(" return mem_get() = %p\n", out));
  return out;
}

/* 'unget' a block of memory */
/*  Note that memory blocks can only be ungot in reverse order to
    how they were got (like a stack) or, alternatively, all
    allocations following the free'd one also be invalid (like an obstack). */
void vt_mem_unget(struct vt_list *mem_list, void *mem)
{
  struct _mem *wn, *nn;
  char *mem_ptr;
  
  d(printf("ungetting memory: %p\n", mem));

  mem_ptr = mem;
  wn = (struct _mem *)mem_list->tailpred;
  nn = wn->prev;
  while (nn) {
    if ((mem_ptr >= wn->mem) && mem_ptr < wn->mem+MEM_BLOCK_SIZE) {
      break;
    }
    wn = nn;
    nn = nn->prev;
  }
  
  if (nn) {
    d(printf("mem_unget: match found!\n"));
    /* remove any following memory blocks */
    while (wn != (struct _mem *)mem_list->tailpred) {
      d(printf("removing tail of list\n"));
      nn = (struct _mem *)vt_list_remtail(mem_list);
      free(nn);
    }
    wn->ptr = mem_ptr - wn->mem;
    d(printf("ptr set to %d\n", wn->ptr));
  }
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

  size = (size+MEM_BLOCK_ALIGN) & (~MEM_BLOCK_ALIGN);	/* long align data */

  d(printf("vt_mem_push called\n"));

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

