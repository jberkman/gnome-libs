/*  memory.h - Zed's Virtual Terminal
 *  Copyright (C) 1998  Michael Zucchi
 *
 *  Exported 'quick' memory functions.
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

#ifndef _MEMORY_H

#include "lists.h"

/* fast memory handling */
int vt_mem_init(struct vt_list *memlist);
void *vt_mem_get(struct vt_list *memlist, int size);
void vt_mem_push(struct vt_list *memlist, void *mem, int size);
void vt_mem_unget(struct vt_list *mem_list, void *mem);

#endif /* _MEMORY_H */
