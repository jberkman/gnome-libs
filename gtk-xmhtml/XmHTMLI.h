/*****
* XmHTMLI.h : XmHTML internal function proto's.
*             Only required when building the XmHTML Library.
*             If you whish to include this file, it *must* be include
*             AFTER XmHTMLP.h as it references a number of structures defined
*             in that header.
*
* This file Version	$Revision$
*
* Creation date:		Tue Aug 19 16:03:22 GMT+0100 1997
* Last modification: 	$Date$
* By:					$Author$
* Current State:		$State$
*
* Author:				newt
*
* Copyright (C) 1994-1997 by Ripley Software Development 
* All Rights Reserved
*
* This file is part of the XmHTML Widget Library.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU [Library] General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU [Library] General Public
* License along with this library; if not, write to the Free
* Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****/
/*****
* $Source$
*****/
/*****
* ChangeLog 
* $Log$
* Revision 1.1  1997/11/28 03:38:55  gnomecvs
* Work in progress port of XmHTML;  No, it does not compile, don't even try -mig
*
* Revision 1.2  1997/10/23 00:24:46  newt
* XmHTML Beta 1.1.0 release
*
* Revision 1.1  1997/08/30 00:07:31  newt
* Initial Revision
*
*****/ 

#ifndef _XmHTMLI_h_
#define _XmHTMLI_h_

_XFUNCPROTOBEGIN

/****
* parse.c 
****/
/* Raw HTML parser */
extern XmHTMLObject *_XmHTMLparseHTML(XmHTMLWidget html, XmHTMLObject *old_list,
	char *input, XmHTMLWidget dest);

/* expand all escape sequences in the given text */
extern void _XmHTMLExpandEscapes(char *string, Boolean warn);

/* Check the existance of a tag */
extern Boolean _XmHTMLTagCheck(char *attributes, char *tag);

/* Get the value of a tag */
extern char *_XmHTMLTagGetValue(char *attributes, char *tag);

/* Get the numerical value of a tag */
extern int _XmHTMLTagGetNumber(char *attributes, char *tag, int def);

/* Check the value of a tag */
extern Boolean _XmHTMLTagCheckValue(char *attributes, 
	char *tag, char *check);

/* Retrieve the value of the ALIGN attribute on images */
extern Alignment _XmHTMLGetImageAlignment(char *attributes);

/* Retrieve the value of the ALIGN attribute */
extern Alignment _XmHTMLGetHorizontalAlignment(char *attributes, 
	Alignment def_align);

/* Retrieve the value of the VALIGN attribute */
extern Alignment _XmHTMLGetVerticalAlignment(char *attributes);

/***** 
* Returns max. width of a line in the current document or 75% of screen width,
* whatever is the smallest. In pixels.
*****/
extern Dimension _XmHTMLGetMaxLineLength(XmHTMLWidget html);

/* free the given parser tree */
extern void _XmHTMLFreeObjects(XmHTMLObject *objects);

/* create a HTML source document from the given parser tree */
extern String _XmHTMLTextGetString(XmHTMLObject *objects);

/****
* callbacks.c
****/
/* XmNlinkCallback driver */
extern void _XmHTMLLinkCallback(XmHTMLWidget html);

/* XmNanchorTrackCallback driver */
extern void _XmHTMLTrackCallback(XmHTMLWidget html, XEvent *event, 
	XmHTMLAnchor *anchor);

/* XmNactivateCallback driver */
extern void _XmHTMLActivateCallback(XmHTMLWidget html, XEvent *event, 
	XmHTMLAnchor *anchor);

/* XmNdocumentCallback driver */
extern Boolean _XmHTMLDocumentCallback(XmHTMLWidget html, Boolean html32,
	Boolean verified, Boolean balanced, Boolean terminated, int pass_level);

/****
* format.c
****/
/* Create a formatted list of objects */
extern XmHTMLObjectTable *_XmHTMLformatObjects(XmHTMLObjectTable *old_table, 
	XmHTMLAnchor *old_anchor, XmHTMLWidget html);

/* fill and allocate a new anchor */
extern XmHTMLAnchor* _XmHTMLNewAnchor(XmHTMLWidget html, XmHTMLObject *object);

extern XmHTMLObjectTable *_XmHTMLCopyTableObject(XmHTMLObjectTable *src);

/****
* frames.c
*****/
/* create all required HTML frame widgets */
extern Boolean _XmHTMLCreateFrames(XmHTMLWidget old, XmHTMLWidget html);

/* destroy all HTML frame widgets */
extern void _XmHTMLDestroyFrames(XmHTMLWidget html, int nframes);

/* frame creation notifier */
extern TWidget _XmHTMLFrameCreateCallback(XmHTMLWidget html,
	XmHTMLFrameWidget *frame);

/* frame destruction notifier */
extern void _XmHTMLFrameDestroyCallback(XmHTMLWidget html, 
	XmHTMLFrameWidget *frame);

/* check for new frames, destroying any previous frame lists */
extern int _XmHTMLCheckForFrames(XmHTMLWidget html, XmHTMLObject *objects);

/* recompute the frame layout after a widget resize */
extern void _XmHTMLReconfigureFrames(XmHTMLWidget html);

/****
* forms.c
****/
/* start a new form */
extern void _XmHTMLStartForm(XmHTMLWidget html, String attributes);

/* terminate the current form */
extern void _XmHTMLEndForm(XmHTMLWidget html);

/* add an input field to the current form */
extern XmHTMLForm *_XmHTMLFormAddInput(XmHTMLWidget html, String attributes);

/* add a select field to the current form */
extern XmHTMLForm *_XmHTMLFormAddSelect(XmHTMLWidget html, String attributes);

/* add a textArea to the current form */
extern XmHTMLForm *_XmHTMLFormAddTextArea(XmHTMLWidget html,
	String attributes, String text);

/* add an option to the given select form entry */
extern void _XmHTMLFormSelectAddOption(XmHTMLWidget html, XmHTMLForm *entry,
	String attributes, String label);

/* wrapup on the given select form entry */
extern void _XmHTMLFormSelectClose(XmHTMLWidget html, XmHTMLForm *entry);

/* destroy the given form */
extern void _XmHTMLFreeForm(XmHTMLWidget html, XmHTMLFormData *form);

/* collect and submit form data */
extern void _XmHTMLFormActivate(XmHTMLWidget html, XEvent *event,
	XmHTMLForm *entry);

/* reset given form data */
extern void _XmHTMLFormReset(XmHTMLWidget html, XmHTMLForm *entry);

/* form widget traversal */
extern void _XmHTMLProcessTraversal(TWidget w, int direction);

/****
* XmHTML.c
****/
/* return the object of a named anchor, given it's id */
extern XmHTMLObjectTableElement _XmHTMLGetAnchorByValue(XmHTMLWidget html, 
	int anchor_id);

/* return the object of a named anchor, given it's name */
extern XmHTMLObjectTableElement _XmHTMLGetAnchorByName(XmHTMLWidget html, 
	String anchor);

/***** 
* Scroll the visible text area to the given x or y position.
* The Widget w is the scrollbar that needs to be scrolled.
* For vertical scrolling, this should be html->html.vsb, for horizontal
* scrolling it should be html->html.hsb.
*****/
extern void _XmHTMLMoveToPos(TWidget w, XmHTMLWidget html, int value);

/* create an XCC for the given HTML widget if not already done */
extern void _XmHTMLCheckXCC(XmHTMLWidget html);

/****
* paint.c
****/
/* Compute screen layout */
extern void _XmHTMLComputeLayout(XmHTMLWidget html);

/* Pour given paint commands onto the display. */
extern void _XmHTMLPaint(XmHTMLWidget html, XmHTMLObjectTable *start,
	XmHTMLObjectTable *end);

/* restart all frozen animations */
extern void _XmHTMLRestartAnimations(XmHTMLWidget html);

/* refresh an image */
extern void _XmHTMLDrawImage(XmHTMLWidget html, XmHTMLObjectTableElement data,
	int y_offset, Boolean from_timerCB);

/****
* numbers.c
* These functions place their return value in a static buffer which is
* overwritten every time they are called, so be sure to copy the return
* value to some other place if you want to keep the numbers.
****/
/* convert given number to an ascii representation */
extern String ToAsciiLower(int val);
extern String ToAsciiUpper(int val);

/* convert given number to a roman numeral */
extern String ToRomanUpper(int val);
extern String ToRomanLower(int val);

/****
* colors.c
****/
/* allocate and return the named pixel. Return def_pixel if that fails */
extern Pixel _XmHTMLGetPixelByName(XmHTMLWidget html, String color,
	Pixel def_pixel);

/* check name of the given color. Only when XmNstrictHTMLChecking is True. */
extern Boolean _XmHTMLConfirmColor32(char *color);

/* free the colors allocated for the given widget */
extern void _XmHTMLFreeColors(XmHTMLWidget html);

/* Recompute top shadow, bottom shadow & highlight colors */
extern void _XmHTMLRecomputeColors(XmHTMLWidget html);

/* Recompute the highlight color given a background pixel */
extern void _XmHTMLRecomputeHighlightColor(XmHTMLWidget html, Pixel bg_color);

/* add a palette to the widget (used for dithering) */
extern Boolean _XmHTMLAddPalette(XmHTMLWidget html);

/****
* images.c and all image reading sources
****/
/* XmHTMLImage macros */
#define ImageIsBackground(IMG)		((IMG)->options & IMG_ISBACKGROUND)
#define ImageIsInternal(IMG)		((IMG)->options & IMG_ISINTERNAL)
#define ImageIsCopy(IMG)			((IMG)->options & IMG_ISCOPY)
#define ImageIsAnim(IMG)			((IMG)->options & IMG_ISANIM)
#define ImageFrameRefresh(IMG)		((IMG)->options & IMG_FRAMEREFRESH)
#define ImageHasDimensions(IMG)		((IMG)->options & IMG_HASDIMENSIONS)
#define ImageHasState(IMG)			((IMG)->options & IMG_HASSTATE)
#define ImageInfoFreed(IMG)			((IMG)->options & IMG_INFOFREED)
#define ImageDelayedCreation(IMG)	((IMG)->options & IMG_DELAYED_CREATION)
#define ImageIsOrphaned(IMG)		((IMG)->options & IMG_ORPHANED)
#define ImageIsProgressive(IMG)		((IMG)->options & IMG_PROGRESSIVE)

/* XmImageInfo macros */
#define ImageInfoDelayed(INFO)		((INFO)->options & XmIMAGE_DELAYED)
#define ImageInfoFreeLater(INFO)	((INFO)->options & XmIMAGE_DEFERRED_FREE)
#define ImageInfoFreeNow(INFO)		((INFO)->options & XmIMAGE_IMMEDIATE_FREE)
#define ImageInfoScale(INFO)		((INFO)->options & XmIMAGE_ALLOW_SCALE)
#define ImageInfoRGBSingle(INFO)	((INFO)->options & XmIMAGE_RGB_SINGLE)
#define ImageInfoShared(INFO)		((INFO)->options & XmIMAGE_SHARED_DATA)
#define ImageInfoClipmask(INFO)		((INFO)->options & XmIMAGE_CLIPMASK)
#define ImageInfoDelayedCreation(INFO) \
									((INFO)->options & XmIMAGE_DELAYED_CREATION)
#define ImageInfoProgressive(INFO)	((INFO)->options & XmIMAGE_PROGRESSIVE)

/* return type of image */
extern Byte _XmHTMLGetImageType(ImageBuffer *ib);

/* rewind the given image buffer */
#define	RewindImageBuffer(IB)	do{ \
	(IB)->next = (size_t)0; \
	(IB)->curr_pos = (IB)->buffer; \
}while(0)

/* free the given image buffer */
#define FreeImageBuffer(IB) { \
	if((IB)->may_free) { \
		free((IB)->file); \
		free((IB)->buffer); \
		free((IB)); \
		(IB) = NULL; \
	} \
}

/* allocate and initialize a rawImageData structure */
#define AllocRawImage(IMG, W, H) do { \
	IMG = (XmHTMLRawImageData*)malloc(sizeof(XmHTMLRawImageData)); \
	memset(IMG, 0, sizeof(XmHTMLRawImageData)); \
	IMG->cmapsize = 0; \
	IMG->bg = -1; \
	IMG->width = W; \
	IMG->height = H; \
	IMG->data = (Byte*)calloc(W*H, sizeof(Byte)); \
	IMG->delayed_creation = False; \
	IMG->color_class = XmIMAGE_COLORSPACE_INDEXED; \
}while(0)

#ifdef WITH_MOTIF
/* allocate a colormap for the given rawImageData */
#define AllocRawImageCmap(IMG,SIZE) do { \
	int i; \
	IMG->cmap = (XColor*)calloc(SIZE, sizeof(XColor)); \
	for(i = 0; i < SIZE; i++) { \
		IMG->cmap[i].pixel = i; IMG->cmap[i].flags = DoRed|DoGreen|DoBlue; } \
	IMG->cmapsize = SIZE; \
}while(0)
#else
	
#define AllocRawImageCmap(IMG,SIZE) do { \
	int i; \
	IMG->cmap = (GdkColor*)calloc(SIZE, sizeof(GdkColor)); \
	for(i = 0; i < SIZE; i++) { \
		IMG->cmap[i].pixel = i; } \
	IMG->cmapsize = SIZE; \
}while(0)
#endif
	
/* allocate and initialize a rawImageData structure with a colormap */
#define AllocRawImageWithCmap(IMG, W, H, SIZE) do { \
	IMG = (XmHTMLRawImageData*)malloc(sizeof(XmHTMLRawImageData)); \
	memset(IMG, 0, sizeof(XmHTMLRawImageData)); \
	AllocRawImageCmap(IMG,SIZE); \
	IMG->bg = -1; \
	IMG->width = W; \
	IMG->height = H; \
	IMG->data = (Byte*)calloc(W*H, sizeof(Byte)); \
	IMG->delayed_creation = False; \
}while(0)

/* destroy allocated image. Only to be called upon error */
#define FreeRawImage(IMG) do{ \
	if(IMG != NULL) { \
		if(IMG->data) free(IMG->data); \
		if(IMG->cmap) free(IMG->cmap); \
		free(IMG); \
		IMG = NULL; \
	}\
}while(0)

/* reset a rawImageData structure */
#define ResetRawImage(IMG) do { \
	memset(IMG, 0, sizeof(XmHTMLRawImageData)); \
	if(IMG->cmap) free(IMG->cmap); /* erase existing colormap */ \
	IMG->cmap = (XColor*)NULL; \
	IMG->cmapsize = 0; \
	IMG->bg = -1; \
	IMG->width = 0; \
	IMG->height = 0; \
	IMG->data = (Byte*)NULL; \
	IMG->delayed_creation = False; \
}while(0)

/* read a file in a buffer */
extern ImageBuffer *_XmHTMLImageFileToBuffer(String file);

/* read an X11 bitmap */
extern XmHTMLRawImageData *_XmHTMLReadBitmap(TWidget html, ImageBuffer *ib);

/* read a GIF file */
extern XmHTMLRawImageData *_XmHTMLReadGIF(TWidget html, ImageBuffer *ib);

/* read a FLG file (Fast Loadable Graphic) */
extern XmImageInfo *_XmHTMLReadFLG(XmHTMLWidget html, ImageBuffer *ib);

/* read len chars from ib to buf */
extern size_t _XmHTMLGifReadOK(ImageBuffer *ib, unsigned char *buf, int len);

/* read the next block of raster data in buf and return no of copied chars */
extern size_t _XmHTMLGifGetDataBlock(ImageBuffer *ib, unsigned char *buf);

/* check whether a GIF is animated or not */
extern int _XmHTMLIsGifAnimated(ImageBuffer *fd);

/* Initialize gif animation reading */
extern int _XmHTMLGifAnimInit(TWidget html, ImageBuffer *ib,
	XmHTMLRawImageData *data);

/* read a frame from an animated gif file */
extern Boolean _XmHTMLGifAnimNextFrame(ImageBuffer *ib,
	XmHTMLRawImageData *data, int *x, int *y, int *timeout, int *dispose);

/* wrap up animated gif reading */
extern void _XmHTMLGifAnimTerminate(ImageBuffer *ib);

/* read an X11 XPM image */
extern XmHTMLRawImageData *_XmHTMLReadXPM(TWidget html, ImageBuffer *ib);

/* read an X11 XPM image from raw XPM data */
extern XmHTMLRawImageData *_XmHTMLCreateXpmFromData(TWidget html, char **data,
	String src);

/* read a PNG image */
extern XmHTMLRawImageData *_XmHTMLReadPNG(TWidget html, ImageBuffer *ib);

/* reread a png image (only used for rgb + alpha channel) */
extern XmHTMLRawImageData *_XmHTMLReReadPNG(XmHTMLWidget html,
	XmHTMLRawImageData *raw_data, int x, int y, Boolean is_body_image);

/* read a JPEG image */
extern XmHTMLRawImageData *_XmHTMLReadJPEG(TWidget html, ImageBuffer *ib);

/* create a new but empty XImage with given dimensions */
extern TXImage *_XmHTMLCreateXImage(XmHTMLWidget html, XCC xcc, Dimension width,
	Dimension height, String url);

/* fill the given XImage */
extern void _XmHTMLFillXImage(XmHTMLWidget html, TXImage *ximage, XCC xcc,
	Byte *data, unsigned long *xcolors, int *start, int *end);

/* create a new image */
extern XmHTMLImage *_XmHTMLNewImage(XmHTMLWidget html, String attributes,
	Dimension *width, Dimension *height);

/* update all copies of the given parent image */
extern void _XmHTMLImageUpdateChilds(XmHTMLImage *image);

/* process all images that need rereading (alpha channel processing) */
extern void _XmHTMLImageCheckDelayedCreation(XmHTMLWidget html);

/* create an animation for the given image */
extern void _XmHTMLMakeAnimation(XmHTMLWidget html, XmHTMLImage *image, 
	Dimension width, Dimension height);

/* create a pixmap from the given ImageInfo data */
extern TPixmap _XmHTMLInfoToPixmap(XmHTMLWidget html, XmHTMLImage *image, 
	XmImageInfo *info, Dimension width, Dimension height,
	unsigned long *global_cmap, TPixmap *clip);

/* replace or update an image */
extern XmImageStatus _XmHTMLReplaceOrUpdateImage(XmHTMLWidget html, 
	XmImageInfo *info, XmImageInfo *new_info, XmHTMLObjectTableElement *elePtr);

/* Free private image data */
extern void _XmHTMLFreeImage(XmHTMLWidget html, XmHTMLImage *image);

/* Free external image data */
extern void _XmHTMLFreeImageInfo(XmHTMLWidget html, XmImageInfo *info,
		Boolean external);

/* Free an image and adjust the internal list of images */
extern void _XmHTMLReleaseImage(XmHTMLWidget html, XmHTMLImage *image);

/* load and set the body image */
extern void _XmHTMLLoadBodyImage(XmHTMLWidget html, String url);

/* readGIF external hooks: external gif decoder and decompress command. */
extern XmImageGifProc XmImageGifProc_plugin;
extern String XmImageGifzCmd_plugin;

/* XmImage configuration hook */
extern XmImageConfig *_xmimage_cfg;

/*****
* quantize.c
*****/
/* convert a 24bit image to an 8bit paletted one, quantizing if required */
extern void _XmHTMLConvert24to8(Byte *data, XmHTMLRawImageData *img_data,
	int max_colors, Byte mode);

/* quantize the given image data down to max_colors */
extern void _XmHTMLQuantizeImage(XmHTMLRawImageData *img_data, int max_colors);

/* convert RGB to pixel. Upon return, img_data contains a full colormap */
extern void _XmHTMLPixelizeRGB(Byte *rgb, XmHTMLRawImageData *img_data);

/* dither the given image to a fixed palette */
extern void _XmHTMLDitherImage(XmHTMLWidget html, XmHTMLRawImageData *img_data);

/*****
* map.c
*****/
/* create an imagemap */
extern XmHTMLImageMap* _XmHTMLCreateImagemap(String name);

/* store an imagemap */
extern void _XmHTMLStoreImagemap(XmHTMLWidget html, XmHTMLImageMap *map);

/* add an area to an imagemap */
extern void _XmHTMLAddAreaToMap(XmHTMLWidget html, XmHTMLImageMap *map, 
	XmHTMLObject *object);

/* get the named imagemap */
extern XmHTMLImageMap *_XmHTMLGetImagemap(XmHTMLWidget html, String name);

/* return anchor data referenced by the given positions and imagemap */
extern XmHTMLAnchor *_XmHTMLGetAnchorFromMap(XmHTMLWidget html, int x, int y,
	XmHTMLImage *image, XmHTMLImageMap *map);

/* check for possible external imagemaps */
extern void _XmHTMLCheckImagemaps(XmHTMLWidget html);

/* free all imagemaps for a XmHTMLWidget */
extern void _XmHTMLFreeImageMaps(XmHTMLWidget html);

/* draw selection areas around each area in an imagemap */
extern void _XmHTMLDrawImagemapSelection(XmHTMLWidget html, 
	XmHTMLImage *image);

/*****
* plc.c
*****/
/*****
* Creates a PLC object for the given TWidget and object to be loaded
* Type indicates what type of object should be created. It can be
* XmNONE, XmPLC_IMAGE or XmPLC_DOCUMENT.
* Also inserts the given PLC in the plc buffer of the given TWidget.
*****/
extern PLCPtr _XmHTMLPLCCreate(XmHTMLWidget html, TPointer priv_data,
	String url, Byte type);

/*****
* The main PLC cycler. Will call itself as long as there are any outstanding
* PLC's on the plc list of the current TWidget (fed to this routine as the
* call_data).
*****/
#ifdef WITH_MOTIF
extern void _XmHTMLPLCCycler(TPointer call_data, TIntervalId *proc_id);
#else
gint
_XmHTMLPLCCycler(gpointer call_data);
#endif

/* kill and remove all outstanding PLC's */
extern void _XmHTMLKillPLCCycler(XmHTMLWidget html);

/*****
* fonts.c
******/
/* scalable font sizes */
extern int xmhtml_fn_sizes[8];
/* basefont sizes */
extern int xmhtml_basefont_sizes[7];
/* fixed font sizes */
extern int xmhtml_fn_fixed_sizes[2];

/* load or get a font from the font cache */
extern XmHTMLfont *_XmHTMLloadQueryFont(TWidget w, String name, String family,
	int ptsz, Byte style, Boolean *loaded);

/* initialize/select a font cache (each display has a seperate one) */
extern XmHTMLfont *_XmHTMLSelectFontCache(XmHTMLWidget html, Boolean reset);

/*****
* Release all fonts for this TWidget. Will only unload fonts if this is
* the last TWidget using the font cache for the display this TWidget was
* displayed on.
*****/
extern void _XmHTMLUnloadFonts(XmHTMLWidget html);

_XFUNCPROTOEND

/* Don't add anything after this endif! */
#endif /* _XmHTMLI_h_ */
