#include <stdio.h>

#define GEN_MIMEDB 1
#include "gnome-magic.c"

extern GnomeMagicEntry *gnome_magic_parse (const char *filename, int *nents);

int main(int argc, char *argv[])
{
  GnomeMagicEntry *ents = NULL;
  char *filename = NULL, *out_filename;
  int nents;

  char *outmem;
  FILE *f;

  gnomelib_init("gnome-gen-mimedb", VERSION);

  if(argc > 1) {
    if(argv[1][0] == '-') {
      fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
      return 1;
    } else if(g_file_exists(argv[1]))
      filename = argv[1];
  } else
    filename = gnome_config_file("mime-magic");

  if(!filename) {
    printf("Input file does not exist (or unspecified)...\n");
    printf("Usage: %s [filename]\n", argv[0]);
    return 1;
  }

  ents = gnome_magic_parse(filename, &nents);

  if(!nents){
	  fprintf (stderr, "%s: Error parsing the %s file\n", argv [0], filename);
	  return 0;
  }

  out_filename = g_copy_strings(filename, ".dat", NULL);

  f = fopen (out_filename, "w");
  if (f == NULL){
    fprintf (stderr, "%s: Can not create the output file %s\n", argv [0], out_filename);
    return 1;
  }

  if(fwrite(ents, sizeof(GnomeMagicEntry), nents, f) != nents){
    fprintf (stderr, "%s: Error while writing the contents of %s\n", argv [0], out_filename);
    return 1;
  }

  fclose(f);

  return 0;
}
