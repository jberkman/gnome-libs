#ifndef _GNOME_RASTERAPI_H_
#define _GNOME_RASTERAPI_H_

#include <gdk/gdk.h>

typedef struct {
  double r,g,b;
} gnome_RGB24; 

typedef struct {
  double r,g,b;
} gnome_RGBd; 

typedef struct {
  double c,i,e;
} gnome_CIEd; 

typedef struct {
  char r,g,b,a
} gnome_RGBA24;

typedef struct {
  double r,g,b,a;
} gnome_RGBAd;

typedef struct {
  doubel c,i,e,a;
} gnome_CIEAd;

typedef struct {
  char *error_str;
} gnome_rasterapiError;

typedef struct {
  gnome_rasterImage *(*open)(gnome_rasterapiCodec *,
			     char *,filename,int flags);
  int (*getImageTileRGB24)(gnome_rasterImage *,
			   int xo,int yo,int width,
			   int height,int layer);
  int (*getImageTileRGBA24)(gnome_rasterImage *,
			   int xo,int yo,int width,
			   int height,int layer);
  void (*close)(gnome_rasterImage *);
} gnome_rasterapiCodec;

typedef struct {
  char *filename;
  char *description;
  int nlayers;
  gnome_rasterapiCodec *codec;
  void *codec_data;
} gnome_rasterImage;

int gnomeGetRasterImageTileRGB24(gnome_rasterImage *, int xo, int yo, 
				 int width, int height, int layer);

int gnomeGetRasterImageTileRGBA24(gnome_rasterImage *, int xo, int yo, 
				  int width, int height, int layer);

#endif /* _GNOME_RASTERAPI_H_ */
