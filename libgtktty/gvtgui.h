/*  GemVt - GNU Emulator of a Virtual Terminal
 *  Copyright (C) 1997  Tim Janik
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
#ifndef __GVTGUI_H__
#define __GVTGUI_H__

#include	<gtk/gtk.h>
#include	"gtktty.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


typedef	enum
{
  GVT_STATE_NONE,
  GVT_STATE_RUNNING,
  GVT_STATE_DEAD
} GvtStateType;


GtkWidget*	gvt_status_bar_new	(GtkWidget	*parent,
					 GtkTty		*tty);
void		gvt_status_bar_update	(GtkWidget	*status_bar);






#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */


#endif /* __GVTGUI_H__ */
