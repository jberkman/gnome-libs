/*###################################################################*/
/*##                    gdk_imlib raw rgb converter                ##*/
/*##                                                               ##*/
/*## This software falls under the GNU Public License. Please read ##*/
/*##              the COPYING file for more information            ##*/
/*###################################################################*/

#include "convertrgb.h"

static int efficient;

static int is_file(char *s)
{
   struct stat st;

   if ((!s)||(!*s)) return 0;
   if (stat(s,&st)<0) return 0;
   if (S_ISREG(st.st_mode)) return 1;
   return 0;
}

static void convert(char *file)
{
	FILE *f, *sf;
	char outfile[4096];
	char *ptr;
	int x, y;
	int col = 0;
	unsigned char *d;
	int w, h, t;

	t = 0;

	sf = fopen(file, "rb");
	if (!sf) return;

	if (isxpm(file))
		d = _LoadXPM(NULL, file, &w, &h, &t);
#ifdef HAVE_LIBPNG
	else if (ispng(file))
		d = _LoadPNG(NULL, sf, &w, &h, &t);
#endif
#ifdef HAVE_LIBJPEG
	else if (isjpeg(file))
		d = _LoadJPEG(NULL, sf, &w, &h);
#endif
#ifdef HAVE_LIBTIFF
	else if (istiff(file))
		d = _LoadTIFF(NULL, file, &w, &h, &t);
#endif
#ifdef HAVE_LIBGIF
	else if (isgif(file))
		d = _LoadGIF(NULL, file, &w, &h, &t);
#endif
	else if (isbmp(file))
		d = _LoadBMP(NULL, file, &w, &h, &t);
	else
		{
		fclose(sf);
		sf = open_helper("%C/convert %s pnm:-", file, "rb");
		d = _LoadPPM(NULL, sf, &w, &h);
		}

	fclose (sf);
	if (!d) return;

	strcpy (outfile, file);
	ptr = outfile + strlen(outfile);
	while (ptr > &outfile[0] && ptr[0] != '.') ptr--;
	ptr[0] = '\0';
	strcpy (ptr, ".c");

	f = fopen(outfile,"w");
	
	if (!f)
		{
		printf("unable to open output file: %s\n", outfile);
		free(d);
		return;
		}

	fprintf(f, "/* Imlib raw rgb data file created by convertrgb */\n\n");

	strcpy (ptr, "_rgb");
	fprintf(f, "static const int %s_width  = %d;\n", outfile, w);
	fprintf(f, "static const int %s_height = %d;\n", outfile, h);
	if (t)
		fprintf(f, "static const GdkImlibColor %s_alpha  = { 255, 0, 255, 0 };\n", outfile);
	else
		fprintf(f, "static const GdkImlibColor %s_alpha  = { -1, -1, -1, 0 };\n", outfile);

	fprintf(f, "static const char %s[] = {\n", outfile);

	for (y=0;y < h; y++)
		for (x=0;x < w; x++)
			{
			unsigned int r, g, b;
			int l;
			l = (( y * w) + x ) * 3;
			r = d[l];
			l++;
			g = d[l];
			l++;
			b = d[l];
			if (!efficient)
				fprintf(f, "0x%.2x, 0x%.2x, 0x%.2x", r, g, b);
			else
				fprintf(f, "%d,%d,%d", r, g, b);
			col++;
			if (y != h -1 || x != w - 1)
				fprintf(f, ",");
			if (col > 3)
				{
				col = 0;
				fprintf(f , "\n");
				}
			else
				{
				if (!efficient) fprintf(f, " ");
				}
			}

	if (col == 0)
		fprintf (f, "};\n");
	else
		fprintf (f, "\n};\n");

	fclose(f);
	free(d);
}

int main (int argc, char *argv[])
{
	efficient = 0;

	if (argc > 1)
		{
		int i = 1;
		while (i < argc)
			{
			char *file = argv[i];
			if (strcmp(file, "--efficient") == 0 || strcmp(file, "-e") == 0)
				efficient = 1;
			else if (is_file(file))
				convert(file);
			else
				printf("Error, file not found: %s\n", file);
			i++;
			}
		}
	else
		{
		printf ("Image to Imlib raw rgb data Converter                 Version 0.1.0\n");
		printf ("This program is released under the terms of the GNU public license.\n");
		printf ("Command line params:\n\n");
		printf ("         convertrgb [-e] [-o=fn|-a=fn] inputfile [inputfile] ...\n\n");
		printf ("   -e, --efficient      Use smallest format possible to save space\n");
		printf ("   -o=[fn]              Output to file named fn , default is .c extension\n");
		printf ("                          to replace extension of inputfile(FIXME)\n");
		printf ("   -a=[fn]              Like -o, but appends to file named fn (FIXME)\n");
		}
	return 0;
}

