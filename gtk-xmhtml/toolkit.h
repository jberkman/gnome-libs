#ifndef TOOLKIT_H
#define TOOLKIT_H

#ifndef WITH_MOTIF
#define __GTK__
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
enum {
	TSTRING_DIRECTION_R_TO_L,
	TSTRING_DIRECTION_L_TO_R
};

enum {
	TALIGNMENT_END,
	TALIGNMENT_CENTER,
	TALIGNMENT_BEGINNING
};

#define _XFUNCPROTOBEGIN
#define _XFUNCPROTOEND

#define TNone        NULL
#define TXImage      GdkImage
#define TIdleKeep    TRUE
#define TIdleRemove  FALSE
#define TNullTimeout 0
#define TIntervalId  int
#define TEvent       GdkEvent

typedef void *TPointer;

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

typedef GdkFont     TFontStruct;
typedef GdkCursor   *TCursor;
typedef GdkVisual   *TVisual;
typedef GdkColormap *TColormap;
typedef GdkPixmap   *TPixmap;
typedef GdkWindow   *TWindow;
typedef GtkWidget   *TAppContext;
typedef GtkWidget   *TWidget;
typedef GdkDrawable *TDrawable;
typedef GdkGC       *TGC;
typedef GList       *TCallbackList;
typedef TWidget     *TWidgetList;
typedef GdkAtom     *TAtom;
typedef GdkColor    TColor;

typedef XVisualInfo TVisualInfo;

#define TLineSolid      GDK_LINE_SOLID
#define TLineDoubleDash GDK_LINE_DOUBLE_DASH
#define TCapButt        GDK_CAP_BUTT
#define TJoinBevel      GDK_JOIN_BEVEL
#define TJoinRound      GDK_JOIN_ROUND

#define Toolkit_Is_Realized(w) GTK_WIDGET_REALIZED(w)
#define Toolkit_Widget_Window(x) (x)->window
#define Toolkit_Default_Root_Window(dpy) ((GdkWindow*) &gdk_root_parent)
#define Toolkit_Pointer_Ungrab(display,time) gdk_pointer_ungrab(time)
#define Toolkit_CurrentTime GDK_CURRENT_TIME
#define Toolkit_HTML_Widget(widget,field) GTK_HTML((widget))->(field)
#define Toolkit_Screen_Width(w) gdk_screen_width ()
#define Toolkit_Display(w) GDK_DISPLAY ()
#define Toolkit_Free_Font(dpy,font) gdk_font_free ((font))
#define Toolkit_Free_Cursor(dpy,cursor) gdk_cursor_destroy ((cursor));

#define Toolkit_Widget_Name(w) "SomeWidget"
#define Toolkit_Set_Font(dpy,gc,xfont) gdk_gc_set_font ((gc), (xfont))
#define Toolkit_Set_Foreground(dpy,gc,fg) do{TColor m;m.pixel=(fg);gdk_gc_set_foreground((gc),&m);}while(0)
#define Toolkit_Set_Line_Attributes(dpy,gc,w,line,cap,join) gdk_gc_set_line_attributes((gc),(w),\
									       (line),(cap),(join))
#define Toolkit_Draw_String(dpy,win,gc,xs,ys,text,len,f) gdk_draw_text((win),(f),(gc),(xs),(ys),(text),(len))
#define Toolkit_Fill_Rectangle(dpy,win,gc,x,y,w,h) gdk_draw_rectangle ((win),(gc),TRUE,(x),(y),(w),(h))
#define Toolkit_Draw_Rectangle(dpy,win,gc,x,y,w,h) gdk_draw_rectangle ((win),(gc),FALSE,(x),(y),(w),(h))
#define Toolkit_Draw_Line(dpy,win,gc,x1,y1,x2,y2) gdk_draw_line ((win),(gc),(x1),(y1),(x2),(y2))
#define Toolkit_Draw_Arc(dpy,win,gc,x,y,w,h,a1,a2) gdk_draw_arc ((win),(gc),FALSE,(x),(y),(w),(h),(a1),(a2))
#define Toolkit_Fill_Arc(dpy,win,gc,x,y,w,h,a1,a2) gdk_draw_arc ((win),(gc),TRUE,(x),(y),(w),(h),(a1),(a2))
#define Toolkit_Text_Width(font,text,len) gdk_text_width (font, text, len)
#define Toolkit_XFont(font) ((XFontStruct *)(((GdkFontPrivate *)font)->xfont))
#define Toolkit_Copy_Area(dpy,src,dst,gc,sx,sy,w,h,dx,dy) \
	gdk_window_copy_area ((dst),(gc),(dx),(dy),(src),(sx),(sy),(w),(h))
#define Toolkit_Create_Pixmap(dpy,win,w,h,d) gdk_pixmap_new((win),(w),(h),(d))
#define Toolkit_GC_Free(dpy,gc) gdk_gc_destroy(gc)
#define Toolkit_Widget_Force_Repaint(w) gtk_widget_draw (GTK_WIDGET (w), NULL)
#define TOolkit_Widget_Repaint(w) gtk_widget_draw (GTK_WIDGET (w), NULL)
#define Toolkit_StyleGC_BottomShadow(w) (GTK_WIDGET(w))->style->dark_gc [GTK_STATE_NORMAL]
#define Toolkit_StyleGC_TopShadow(w)    (GTK_WIDGET(w))->style->light_gc [GTK_STATE_NORMAL]
#define Toolkit_StyleGC_Highlight(w)    (GTK_WIDGET(w))->style->bg_gc [GTK_STATE_PRELIGHT]
#define Toolkit_StyleColor_Highlight(w)    (GTK_WIDGET(w))->style->bg [GTK_STATE_PRELIGHT].pixel
#define Toolkit_Widget_Dim(h) (GTK_WIDGET(h)->allocation)
#define Toolkit_Screen_Height(w) gdk_screen_height ()
#define Toolkit_Widget_Is_Realized(w) GTK_WIDGET_REALIZED (w)
#define Toolkit_Clear_Area(d,w,xs,ys,wi,h) gdk_window_clear_area ((w),(xs),(ys),(wi),(h));
#else

#define TNone        None
#define TPointer     XtPointer
#define TColor       XColor
#define TPixmap      Pixmap
#define TWindow      Window
#define TXImage      XImage
#define TIdleKeep    False
#define TIdleRemove  True
#define TNullTimeout None
#define TEvent       XEvent
#define TCallbackList XtCallbackList
#define TIntervalId  XtIntervalId
#define TAppContext  XtAppContext
#define TGC          GC
#define TFontStruct  XFontStruct
#define TWidgetList  WidgetList
#define TSTRING_DIRECTION_R_TO_L XmSTRING_DIRECTION_R_TO_L
#define TSTRING_DIRECTION_L_TO_R XmSTRING_DIRECTION_L_TO_R

#define TLineSolid      LineSolid      
#define TLineDoubleDash LineDoubleDash 
#define TCapButt        CapButt        
#define TJoinBevel      JoinBevel      
#define TJoinRound      JoinRound

#define Toolkit_Is_Realized(w) XtIsRealized ((Widget) w)
#define Toolkit_Widget_Window(x) XtWindow((x))
#define Toolkit_Default_Root_Window(dpy) DefaultRootWindow(dpy)
#define Toolkit_Pointer_Ungrab(display,time) XUngrabPointer(display,time)
#define Toolkit_CurrentTime CurrentTime
#define Toolkit_HTML_Widget(widget,field) (widget)->html.(field)
#define Toolkit_Screen_Width(w) WidthOfScreen(XtScreen((Widget)w)));
#define Toolkit_Display(w) XtDisplay(w)
#define Toolkit_Free_Font(dpy,font) XFreeFont (dpy, (font))
#define Toolkit_Widget_Name(w) XtName(w)
#define Toolkit_Set_Font(dpy,gc,xfont) XSetFont ((dpy), (gc), (xfont)->fid)
#define Toolkit_Set_Foreground(dpy,gc,fg) XSetForeground((dpy),(gc),(fg))
#define Toolkit_Set_Line_Attributes(dpy,gc,w,line,cap,join) XSetLineAttributes((dpy),(gc),(w),\
									       (line),(cap),(join))
#define Toolkit_Draw_String(dpy,win,gc,xs,ys,text,len,f) XDrawString((dpy),(win),(gc),(xs),(ys),(text),(len))
#define Toolkit_Fill_Rectangle(dpy,win,gc,x,y,w,h) XFillRectangle ((dpy),(win),(gc),(x),(y),(w),(h))
#define Toolkit_Draw_Rectangle(dpy,win,gc,x,y,w,h) XDrawRectangle ((dpy),(win),(gc),(x),(y),(w),(h))
#define Toolkit_Draw_Line(dpy,win,gc,x1,y1,x2,y2) XDrawLine ((dpy),(win),(gc),(x1),(y1),(x2),(y2))
#define Toolkit_Draw_Arc(dpy,win,gc,x,y,w,h,a1,a2) XDrawArc ((dpy),(win),(gc),(x),(y),(w),(h),(a1),(a2))
#define Toolkit_Fill_Arc(dpy,win,gc,x,y,w,h,a1,a2) XFillArc ((dpy),(win),(gc),(x),(y),(w),(h),(a1),(a2))

#define Toolkit_Text_Width(font,text,len) XTextWidth (font, text, len)
#define Toolkit_XFont(font) font
#define Toolkit_Copy_Area(dpy,src,dst,gc,sx,sy,w,h,dx,dy) \
     XCopyArea ((dpy),(src),(dst),(gc),(sx),(sy),(w),(h),(dx),(dy))
#define Toolkit_Create_Pixmap(dpy,win,w,h,d) XCreatePixmap((dpy),(win),(w),(h),(d))
#define Toolkit_GC_Free(dpy,gc) XFreeGC((dpy),(gc))
#define Toolkit_Free_Cursor(dpy,cursor) XFreeCursor ((dpy), (cursor))
#define Toolkit_Widget_Repaint(w) ClearArea((w), 0, 0, (w)->core.width, (w)->core.height)
#define Toolkit_Widget_Force_Repaint(w) \
	do { ClearArea((w), 0, 0, (w)->core.width, (w)->core.height); \
	XSync(XtDisplay((TWidget)(w)), True); } while (0)

#define Toolkit_StyleGC_BottomShadow(w) (w)->manager.bottom_shadow_GC
#define Toolkit_StyleGC_TopShadow(w) (w)->manager.top_shadow_GC
#define Toolkit_StyleGC_Highlight(w) (w)->manager.highlight_GC
#define Toolkit_StyleColor_Highlight(w)    (w)->manager.highlight_color
#define Toolkit_Widget_Dim(h) ((h)->core)
#define Toolkit_Screen_Height(w) HeightOfScreen(w)
#define Toolkit_Widget_Is_Realized(w) XtIsRealized (w)
#define Toolkit_Clear_Area (d,w,xs,ys,w,h) XClearArea ((d),(w),(xs),(ys),(w),(h), False);
	     
#define	TALIGNMENT_END       XmALIGNMENT_END 
#define TALIGNMENT_CENTER    XmALIGNMENT_CENTER
#define TALIGNMENT_BEGINNING XmALIGNMENT_BEGINNING

#endif

#endif

