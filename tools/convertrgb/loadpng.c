/* PNG loader copied from Imlib 1.8 (load.c)
 * with a few modifications 
 */

#include "convertrgb.h"
#include <glib.h>

static int transp;

#ifdef HAVE_LIBPNG
/* this code is cut-and-pasted from gdk-pixbuf; if someone wants
   to fool with the build issues, we could just use gdk-pixbuf
*/
static void
setup_png_transformations(png_structp png_read_ptr, png_infop png_info_ptr,
                          gboolean *fatal_error_occurred,
                          png_uint_32* width_p, png_uint_32* height_p,
                          int* color_type_p)
{
        png_uint_32 width, height;
        int bit_depth, color_type, interlace_type, compression_type, filter_type;
        int channels;
        
        /* Get the image info */

        png_get_IHDR (png_read_ptr, png_info_ptr,
                      &width, &height,
                      &bit_depth,
                      &color_type,
                      &interlace_type,
                      &compression_type,
                      &filter_type);

        /* set_expand() basically needs to be called unless
           we are already in RGB/RGBA mode
        */
        if (color_type == PNG_COLOR_TYPE_PALETTE &&
            bit_depth <= 8) {

                /* Convert indexed images to RGB */
                png_set_expand (png_read_ptr);

        } else if (color_type == PNG_COLOR_TYPE_GRAY &&
                   bit_depth < 8) {

                /* Convert grayscale to RGB */
                png_set_expand (png_read_ptr);

        } else if (png_get_valid (png_read_ptr, 
                                  png_info_ptr, PNG_INFO_tRNS)) {

                /* If we have transparency header, convert it to alpha
                   channel */
                png_set_expand(png_read_ptr);
                
        } else if (bit_depth < 8) {

                /* If we have < 8 scale it up to 8 */
                png_set_expand(png_read_ptr);


                /* Conceivably, png_set_packing() is a better idea;
                 * God only knows how libpng works
                 */
        }

        /* If we are 16-bit, convert to 8-bit */
        if (bit_depth == 16) {
                png_set_strip_16(png_read_ptr);
        }

        /* If gray scale, convert to RGB */
        if (color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
                png_set_gray_to_rgb(png_read_ptr);
        }
        
        /* If interlaced, handle that */
        if (interlace_type != PNG_INTERLACE_NONE) {
                png_set_interlace_handling(png_read_ptr);
        }
        
        /* Update the info the reflect our transformations */
        png_read_update_info(png_read_ptr, png_info_ptr);
        
        png_get_IHDR (png_read_ptr, png_info_ptr,
                      &width, &height,
                      &bit_depth,
                      &color_type,
                      &interlace_type,
                      &compression_type,
                      &filter_type);

        *width_p = width;
        *height_p = height;
        *color_type_p = color_type;
        
#ifndef G_DISABLE_CHECKS
        /* Check that the new info is what we want */
        
        if (bit_depth != 8) {
                g_warning("Bits per channel of transformed PNG is %d, not 8.", bit_depth);
                *fatal_error_occurred = TRUE;
                return;
        }

        if ( ! (color_type == PNG_COLOR_TYPE_RGB ||
                color_type == PNG_COLOR_TYPE_RGB_ALPHA) ) {
                g_warning("Transformed PNG not RGB or RGBA.");
                *fatal_error_occurred = TRUE;
                return;
        }

        channels = png_get_channels(png_read_ptr, png_info_ptr);
        if ( ! (channels == 3 || channels == 4) ) {
                g_warning("Transformed PNG has %d channels, must be 3 or 4.", channels);
                *fatal_error_occurred = TRUE;
                return;
        }
#endif
}

unsigned char      *
g_LoadPNG(FILE * f, int *wp, int *hp, int *t)
{
	png_structp png_ptr;
	png_infop info_ptr, end_info;
        gboolean failed = FALSE;
	gint i, ctype, bpp;
	png_uint_32 w, h;
	png_bytepp rows;
	guchar *pixels;

	png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return NULL;

	info_ptr = png_create_info_struct (png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct (&png_ptr, NULL, NULL);
		return NULL;
	}

	end_info = png_create_info_struct (png_ptr);
	if (!end_info) {
		png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
		return NULL;
	}

	if (setjmp (png_ptr->jmpbuf)) {
		png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);
		return NULL;
	}

	png_init_io (png_ptr, f);
	png_read_info (png_ptr, info_ptr);

        setup_png_transformations(png_ptr, info_ptr, &failed, &w, &h, &ctype);

        if (failed) {
                png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);
                return NULL;
        }
        
	if (ctype & PNG_COLOR_MASK_ALPHA)
		bpp = 4;
	else
		bpp = 3;

	pixels = malloc (w * h * bpp);
	if (!pixels) {
		png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);
		return NULL;
	}

	rows = g_new (png_bytep, h);

	for (i = 0; i < h; i++)
		rows[i] = pixels + i * w * bpp;

	png_read_image (png_ptr, rows);
	png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);
	g_free (rows);

	if (ctype & PNG_COLOR_MASK_ALPHA)
          *t = TRUE;
	else
          *t = FALSE;

        *wp = w;
        *hp = h;
        
        return pixels;
}
#endif /* HAVE_LIBPNG */

int
gispng(char *file)
{
#ifdef HAVE_LIBPNG
   FILE               *f;
   unsigned char       buf[8];

   f = fopen(file, "rb");
   if (!f)
      return 0;
   fread(buf, 1, 8, f);
   fclose(f);
   return (int)png_check_sig(buf, 8);
#else
   return 0;
#endif
}
