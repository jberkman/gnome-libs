/*
 * Copyright (C) 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */


/*
 * gtk-clock: The GTK clock widget
 *
 * Author: Szekeres Istv�n (szekeres@cyberspace.mht.bme.hu)
 */

#include <time.h>
#include <gtk/gtk.h>
#include <string.h>
#include "gtk-clock.h"

static void gtk_clock_class_init(GtkClockClass *klass);
static void gtk_clock_init(GtkClock *clock);

static GtkLabelClass *parent_class = NULL;

guint gtk_clock_get_type(void)
{
	static guint gtkclock_type = 0;
	if (!gtkclock_type) {
		GtkTypeInfo gtkclock_info = {
			"GtkClock",
			sizeof(GtkClock),
			sizeof(GtkClockClass),
			(GtkClassInitFunc) gtk_clock_class_init,
			(GtkObjectInitFunc) gtk_clock_init,
			NULL,
			NULL,
			NULL
		};
		gtkclock_type = gtk_type_unique(gtk_label_get_type(), &gtkclock_info);
	}

	return gtkclock_type;
}

static void gtk_clock_destroy(GtkObject *object)
{
	g_return_if_fail(object != NULL);

	/* remember, destroy can be run multiple times! */

	gtk_clock_stop(GTK_CLOCK(object));
	GTK_OBJECT_CLASS(parent_class)->destroy(object);
}

static void gtk_clock_class_init(GtkClockClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *)klass;
	
	object_class->destroy = gtk_clock_destroy;
	parent_class = gtk_type_class(gtk_label_get_type());
}

static void gtk_clock_init(GtkClock *clock)
{
	clock->timer_id  = -1;
	clock->update_interval = 1;
	clock->seconds = time(NULL);
        clock->stopped = 0;
}

static void gtk_clock_gen_str(GtkClock *clock)
{
	gchar timestr[64];
	time_t secs;

	switch (clock->type) {
	case GTK_CLOCK_DECREASING:
                secs = clock->seconds-time(NULL);
		break;
	case GTK_CLOCK_INCREASING:
                secs = time(NULL)-clock->seconds;
		break;
	case GTK_CLOCK_REALTIME:
		secs = time(NULL);
		break;
	}


	if (clock->type == GTK_CLOCK_REALTIME) {
		clock->tm = localtime(&secs);
	} else {
		clock->tm->tm_hour = secs/3600;
		secs -= clock->tm->tm_hour*3600;
		clock->tm->tm_min = secs/60;
		clock->tm->tm_sec = secs - clock->tm->tm_min*60;
	}
	
	strftime(timestr, 64, clock->fmt, clock->tm);
	gtk_label_set_text(GTK_LABEL(clock), timestr);
}

static gint gtk_clock_timer_callback(gpointer data)
{
	GtkClock *clock = (GtkClock *)data;

	GDK_THREADS_ENTER ();
	gtk_clock_gen_str(clock);
	GDK_THREADS_LEAVE ();

	return TRUE;
}

static gint gtk_clock_timer_first_callback(gpointer data)
{
	GtkClock *clock = (GtkClock *)data;
        gint tmpid;

	GDK_THREADS_ENTER();
	
	gtk_clock_gen_str(clock);

	tmpid =  gtk_timeout_add(1000*clock->update_interval,
				 gtk_clock_timer_callback, clock);

	gtk_clock_stop(clock);

	clock->timer_id = tmpid;

	GDK_THREADS_LEAVE();

	return FALSE;
}

void gtk_clock_construct(GtkClock *gclock, GtkClockType type)
{
	g_return_if_fail(gclock != NULL);
	g_return_if_fail(GTK_IS_CLOCK(gclock));

	gclock->type = type;
	
	if (type == GTK_CLOCK_REALTIME) {
		gclock->fmt = g_strdup("%H:%M");
		gclock->update_interval = 60;
		gclock->tm = localtime(&gclock->seconds);
		gclock->timer_id = gtk_timeout_add(1000*(60-gclock->tm->tm_sec),
						   gtk_clock_timer_first_callback, gclock);
	} else {
		gclock->fmt = g_strdup("%H:%M:%S");
		gclock->tm = g_new(struct tm, 1);
  		memset(gclock->tm, 0, sizeof(struct tm));
		gclock->update_interval = 1;
	}

	gtk_clock_gen_str(gclock);
}

GtkWidget *gtk_clock_new(GtkClockType type)
{
	GtkClock *clock = gtk_type_new(gtk_clock_get_type());

	gtk_clock_construct (clock, type);
	return GTK_WIDGET(clock);
}

void gtk_clock_set_format(GtkClock *clock, const gchar *fmt)
{
	g_return_if_fail(clock != NULL);
	g_return_if_fail(fmt != NULL);
	
	g_free(clock->fmt);
	clock->fmt = g_strdup(fmt);
}

void gtk_clock_set_seconds(GtkClock *clock, time_t seconds)
{
	g_return_if_fail(clock != NULL);

	if (clock->type == GTK_CLOCK_INCREASING) {
		clock->seconds = time(NULL)-seconds;
	} else if (clock->type == GTK_CLOCK_DECREASING) {
		clock->seconds = time(NULL)+seconds;
	}
	if (clock->timer_id == -1) clock->stopped = seconds;
	gtk_clock_gen_str(clock);
}

void gtk_clock_set_update_interval(GtkClock *clock, gint seconds)
{
	guint tmp;

	g_return_if_fail(clock != NULL);

	tmp = clock->update_interval;

	clock->update_interval = seconds;

	if (tmp > seconds && clock->timer_id != -1) {
		gtk_clock_stop(clock);
		gtk_clock_start(clock);
	}
}

void gtk_clock_start(GtkClock *clock)
{
	g_return_if_fail(clock != NULL);

        if (clock->timer_id != -1) return;

        gtk_clock_set_seconds(clock, clock->stopped);
	clock->timer_id = gtk_timeout_add(1000*clock->update_interval, gtk_clock_timer_callback, clock);
}

void gtk_clock_stop(GtkClock *clock)
{
	g_return_if_fail(clock != NULL);

	if (clock->timer_id == -1)
		return;

	if (clock->type == GTK_CLOCK_INCREASING) {
		clock->stopped = time(NULL)-clock->seconds;
	} else if (clock->type == GTK_CLOCK_DECREASING) {
		clock->stopped = clock->seconds-time(NULL);
	}

	gtk_timeout_remove(clock->timer_id);
        clock->timer_id = -1;
}
