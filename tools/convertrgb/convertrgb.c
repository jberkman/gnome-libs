/*###################################################################*/
/*##                    gdk_imlib raw rgb converter                ##*/
/*##                                                               ##*/
/*## This software falls under the GNU Public License. Please read ##*/
/*##              the COPYING file for more information            ##*/
/*###################################################################*/

#include "convertrgb.h"

static int efficient;
static int dimensions;
static int append;
static char *file_for_output;
static char *suffix;
static char *variable_name;

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
	char data_var[4096];
	char *ptr;
	int x, y;
	int col = 0;
	unsigned char *d;
	int w, h, t;

	t = 0;


	if (file_for_output)
		{
		strcpy(outfile, file_for_output);
		}
	else
		{
		strcpy (outfile, file);
		ptr = outfile + strlen(outfile);
		while (ptr > &outfile[0] && ptr[0] != '.') ptr--;
		ptr[0] = '\0';
		if (suffix)
			strcpy (ptr, suffix);
		else
			strcpy (ptr, ".c");
		}

	if (variable_name)
		strcpy(data_var, variable_name);
	else
		{
		strcpy(data_var, file);
		ptr = data_var + strlen(data_var);
		while (ptr > &data_var[0] && ptr[0] != '.') ptr--;
		strcpy (ptr, "_rgb");
		}

	if (strcmp(file, outfile) == 0)
		{
		printf("Input and output files cannot be the same!\n");
		return;
		}

	sf = fopen(file, "rb");
	if (!sf) return;

	if (gisxpm(file) && 0) /* imlib needs display to load xpms, so use convert */
		d = g_LoadXPM(file, &w, &h, &t);
#ifdef HAVE_LIBPNG
	else if (gispng(file))
		d = g_LoadPNG(sf, &w, &h, &t);
#endif
#ifdef HAVE_LIBJPEG
	else if (gisjpeg(file))
		d = g_LoadJPEG(sf, &w, &h);
#endif
#ifdef HAVE_LIBTIFF
	else if (gistiff(file))
		d = g_LoadTIFF(file, &w, &h, &t);
#endif
#ifdef HAVE_LIBGIF
	else if (gisgif(file))
		d = g_LoadGIF(file, &w, &h, &t);
#endif
	else if (gisbmp(file))
		d = g_LoadBMP(file, &w, &h, &t);
	else
		{
		fclose(sf);
		sf = open_helper("%C/convert %s pnm:-", file, "rb");
		d = g_LoadPPM(sf, &w, &h);
		}

	fclose (sf);
	if (!d) return;

	if (append)
		f = fopen(outfile,"a");
	else
		f = fopen(outfile,"w");
	
	if (!f)
		{
		printf("unable to open output file: %s\n", outfile);
		free(d);
		return;
		}

	fprintf(f, "/* Imlib raw rgb data file created by convertrgb */\n\n");

	if (dimensions)
		{
		fprintf(f, "static const int %s_width  = %d;\n", data_var, w);
		fprintf(f, "static const int %s_height = %d;\n", data_var, h);
		if (t)
			fprintf(f, "static const GdkImlibColor %s_alpha  = { 255, 0, 255, 0 };\n", data_var);
		else
			fprintf(f, "static const GdkImlibColor %s_alpha  = { -1, -1, -1, 0 };\n", data_var);
		}

	fprintf(f, "static const char %s[] = {\n", data_var);

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
	dimensions = 1;
	append = 0;
	file_for_output = NULL;
	suffix = NULL;
	variable_name = NULL;

	if (argc > 1)
		{
		int i = 1;
		while (i < argc)
			{
			char *file = argv[i];
			if (strcmp(file, "--efficient") == 0 || strcmp(file, "-e") == 0)
				efficient = 1;
			else if (strcmp(file, "--nodim") == 0 || strcmp(file, "-n") == 0)
				dimensions = 0;
			else if (strncmp(file, "-o", 2) == 0)
				{
				if (append || file_for_output || suffix)
					{
					printf("Cannot Specify multiple output methods of -a, -o, or -s\n");
					return 1;
					}
				if (strlen(file) > 3)
					{
					file_for_output = strdup(file + 3);
					}
				else
					{
					printf("No output file specified for -o option\n");
					return 1;
					}
				}
			else if (strncmp(file, "-a", 2) == 0)
				{
				if (append || file_for_output || suffix)
					{
					printf("Cannot Specify multiple output methods of -a, -o, or -s\n");
					return 1;
					}
				if (strlen(file) > 3)
					{
					append = 1;
					file_for_output = strdup(file + 3);
					}
				else
					{
					printf("No output file specified for -a option\n");
					return 1;
					}
				}
			else if (strncmp(file, "-s", 2) == 0)
				{
				if (append || file_for_output || suffix)
					{
					printf("Cannot Specify multiple output methods of -a, -o, or -s\n");
					return 1;
					}
				if (strlen(file) > 3)
					{
					append = 1;
					suffix = strdup(file + 3);
					}
				else
					{
					printf("No suffix specified for -s option\n");
					return 1;
					}
				}
			else if (strncmp(file, "-v", 2) == 0)
				{
				if (variable_name)
					{
					printf("Cannot -v more than once before each input file\n");
					return 1;
					}
				if (strlen(file) > 3)
					{
					variable_name = strdup(file + 3);
					}
				else
					{
					printf("No name specified for -v option\n");
					return 1;
					}
				}
			else if (is_file(file))
				{
				convert(file);
				if (file_for_output && !append)
					{
					free(file_for_output);
					file_for_output = NULL;
					}
				if (variable_name)
					{
					free(variable_name);
					variable_name = NULL;
					}
				}
			else
				printf("Error, file not found: %s\n", file);
			i++;
			}
		}
	else
		{
		printf ("Image to Imlib raw rgb data Converter                 Version 0.1.2\n");
		printf ("This program is released under the terms of the GNU public license.\n");
		printf ("Command line params:\n\n");
		printf ("      convertrgb [-e] [-o=fn|-a=fn|-s=sn] [-v=vn] inputfile [inputfile] ...\n\n");
		printf ("   -e, --efficient      Use smallest format possible to save space\n");
		printf ("   -n, --nodim          Suppress output of dimensions and transparency\n");
		printf ("   -o=[fn]              Output to file named fn , default is .c extension\n");
		printf ("                          to replace extension of inputfile\n");
		printf ("   -a=[fn]              Like -o, but appends to file named fn\n");
		printf ("   -s=[sn]              Suffix for output filename\n");
		printf ("                        (eg: 'convertrgb -s=.rgb logo.png'sets output file\n");
		printf ("                         to 'logo.rgb')\n");
		printf ("   -v=[vn]              The variable name to use for the next image.\n");
		}

	return 0;
}

