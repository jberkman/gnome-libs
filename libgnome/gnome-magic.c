#include <libgnome/libgnome.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

typedef enum {
  T_END /* end of array */, T_BYTE, T_SHORT, T_LONG, T_STR, T_DATE, 
  T_BESHORT, T_BELONG, T_BEDATE,
  T_LESHORT, T_LELONG, T_LEDATE
} GnomeMagicType;

typedef struct _GnomeMagicEntry {
  guint32 mask;
  GnomeMagicType type;
  guint16 offset, level;

  char test[48];
  guchar test_len;
  enum { CHECK_EQUAL, CHECK_LT, CHECK_GT, CHECK_AND, CHECK_XOR,
	 CHECK_ANY } comptype;
  guint32 compval;

  char mimetype[48];
} GnomeMagicEntry;

#ifndef GEN_MIMEDB
/****** misc lame parsing routines *******/
static guchar
read_octal_str(char **pos)
{
  guchar retval = 0;

  if(**pos >= '0' && **pos <= '7') {
    retval += **pos - '0';
  } else
    return retval;

  retval *= 8;
  (*pos)++;

  if(**pos >= '0' && **pos <= '7') {
    retval += **pos - '0';
  } else
    return retval/8;

  (*pos)++;

  retval *= 8;

  if(**pos >= '0' && **pos <= '7') {
    retval += **pos - '0';
  } else
    return retval/8;

  (*pos)++;

  return retval;
}

static char
read_hex_str(char **pos)
{
  char retval = 0;

  if(**pos >= '0' && **pos <= '9') {
    retval = **pos - '0';
  } else if(**pos >= 'a' && **pos <= 'f') {
    retval = **pos - 'a' + 10;
  } else if(**pos >= 'A' && **pos <= 'A') {
    retval = **pos - 'A' + 10;
  } else
    g_error("bad hex digit %c", **pos);

  (*pos)++;
  retval *= 16;

  if(**pos >= '0' && **pos <= '9') {
    retval += **pos - '0';
  } else if(**pos >= 'a' && **pos <= 'f') {
    retval += **pos - 'a' + 10;
  } else if(**pos >= 'A' && **pos <= 'A') {
    retval += **pos - 'A' + 10;
  } else
    g_error("bad hex digit %c", **pos);

  (*pos)++;

  return retval;
}

static char *
read_string_val(char *curpos, char *intobuf, guchar *into_len)
{
  *into_len = 0;

  while(1) {
    if(!*curpos || isspace(*curpos)) {
      *intobuf = '\0';
      return curpos;
    }

    switch(*curpos) {
    case '\\':
      curpos++;
      switch(*curpos) {
      case 'x': /* read hex value */
	curpos++;
	*(intobuf++) = read_hex_str(&curpos);
	break;
      case '0': case '1':
	/* read octal value */
	*(intobuf++) = read_octal_str(&curpos);
	break;
      case 'n': *(intobuf++) = '\n'; curpos++; break;
      case ' ': *(intobuf++) = ' '; curpos++; break;
      case '\\': *(intobuf++) = '\\'; curpos++; break;
      }
      break;
    default:
      *(intobuf++) = *curpos;
      curpos++;
    }
    (*into_len)++;
  }
}

static char *read_num_val(char *curpos, int bsize, char *intobuf)
{
  char fmttype, fmtstr[4];
  short itmp;

  if(*curpos == '0') {
    if(tolower(*(curpos+1)) == 'x') fmttype = 'x';
    else fmttype = 'o';
  } else
    fmttype = 'u';

  switch(bsize) {
  case 1:
    fmtstr[0] = '%'; fmtstr[1] = fmttype; fmtstr[2] = '\0';
    if(sscanf(curpos, fmtstr, &itmp) < 1) return NULL;
    *(guchar *)intobuf = (guchar)itmp;
    break;
  case 2:
    fmtstr[0] = '%'; fmtstr[1] = 'h'; fmtstr[2] = fmttype; fmtstr[3] = '\0';
    if(sscanf(curpos, fmtstr, intobuf) < 1) return NULL;
    break;
  case 4:
    fmtstr[0] = '%'; fmtstr[1] = fmttype; fmtstr[2] = '\0';
    if(sscanf(curpos, fmtstr, intobuf) < 1) return NULL;
    break;
  }

  while(*curpos && !isspace(*curpos)) curpos++;

  return curpos;
}

GnomeMagicEntry *gnome_magic_parse(const char *filename, int *nents)
{
  GArray *array;
  GnomeMagicEntry newent, *retval;
  FILE *infile;
  char *infile_name;
  int bsize;
  char aline[256];
  char *curpos;

  infile_name = filename;

  if(!infile_name)
    return NULL;

  infile = fopen(infile_name, "r");
  if(!infile)
    return NULL;

  array = g_array_new(FALSE, FALSE, sizeof(GnomeMagicEntry));

  while(fgets(aline, sizeof(aline), infile)) {
    curpos = aline;

    while(*curpos && isspace(*curpos)) curpos++; /* eat the head */
    if(!isdigit(*curpos)) continue;

    if(sscanf(curpos, "%hu", &newent.offset) < 1) continue;
    
    while(*curpos && !isspace(*curpos)) curpos++; /* eat the offset */
    while(*curpos && isspace(*curpos)) curpos++; /* eat the spaces */
    if(!*curpos) continue;

    if(!strncmp(curpos, "byte", strlen("byte"))) {
      curpos += strlen("byte");
      newent.type = T_BYTE;
    } else if(!strncmp(curpos, "short", strlen("short"))) {
      curpos += strlen("short");
      newent.type = T_SHORT;
    } else if(!strncmp(curpos, "long", strlen("long"))) {
      curpos += strlen("long");
      newent.type = T_LONG;
    } else if(!strncmp(curpos, "string", strlen("string"))) {
      curpos += strlen("string");
      newent.type = T_STR;
    } else if(!strncmp(curpos, "date", strlen("date"))) {
      curpos += strlen("date");
      newent.type = T_DATE;
    } else if(!strncmp(curpos, "beshort", strlen("beshort"))) {
      curpos += strlen("beshort");
      newent.type = T_BESHORT;
    } else if(!strncmp(curpos, "belong", strlen("belong"))) {
      curpos += strlen("belong");
      newent.type = T_BELONG;
    } else if(!strncmp(curpos, "bedate", strlen("bedate"))) {
      curpos += strlen("bedate");
      newent.type = T_BEDATE;
    } else if(!strncmp(curpos, "leshort", strlen("leshort"))) {
      curpos += strlen("leshort");
      newent.type = T_LESHORT;
    } else if(!strncmp(curpos, "lelong", strlen("lelong"))) {
      curpos += strlen("lelong");
      newent.type = T_LELONG;
    } else if(!strncmp(curpos, "ledate", strlen("ledate"))) {
      curpos += strlen("ledate");
      newent.type = T_LEDATE;
    } else
      continue; /* weird type */

    if(*curpos == '&') {
      curpos++;
      if(!read_num_val(curpos, 4, (char *)&newent.mask))
	continue;
    } else
      newent.mask = 0xFFFFFFFF;

    while(*curpos && isspace(*curpos)) curpos++;

    switch(newent.type) {
    case T_BYTE:
      bsize = 1;
      break;
    case T_SHORT:
    case T_BESHORT:
    case T_LESHORT:
      bsize = 2;
      break;
    case T_LONG:
    case T_BELONG:
    case T_LELONG:
      bsize = 4;
      break;
    case T_DATE:
    case T_BEDATE:
    case T_LEDATE:
      bsize = 4;
      break;
    default:
    }

    if(newent.type == T_STR)
      curpos = read_string_val(curpos, newent.test, &newent.test_len);
    else {
      newent.test_len = bsize;
      curpos = read_num_val(curpos, bsize, newent.test);
    }

    if(!curpos) continue;

    while(*curpos && isspace(*curpos)) curpos++;

    if(!*curpos) continue;

    g_snprintf(newent.mimetype, sizeof(newent.mimetype), "%s", curpos);
    bsize = strlen(newent.mimetype) - 1;
    while(newent.mimetype[bsize] && isspace(newent.mimetype[bsize]))
      newent.mimetype[bsize--] = '\0';

    g_array_append_val(array, newent);
  }

  newent.type = T_END;
  g_array_append_val(array, newent);

  retval = (GnomeMagicEntry *)array->data;
  if(nents)
    *nents = array->len;

  g_array_free(array, FALSE);

  return retval;
}

static void do_byteswap(guchar *outdata,
		 const guchar *data,
		 gulong datalen)
{
  const guchar *source_ptr = data;
  guchar *dest_ptr = (guchar *)outdata + datalen - 1;
  while(dest_ptr >= outdata)
    *dest_ptr-- = *source_ptr++;
}

static gboolean
gnome_magic_matches_p(FILE *fh, GnomeMagicEntry *ent)
{
  char buf[sizeof(ent->test)];
  gboolean retval = TRUE;

  fseek(fh, (long)ent->offset, SEEK_SET);
  fread(buf, ent->test_len, 1, fh);

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  if(ent->type >= T_BESHORT && ent->type <= T_BEDATE)
#else /* assume big endian */
  if(ent->type >= T_LESHORT && ent->type <= T_LEDATE)
#endif
    { /* endian-convert comparison */
      char buf2[sizeof(ent->test)];

      do_byteswap(buf2, buf, ent->test_len);
      retval &= !memcmp(ent->test, buf2, ent->test_len);
    }
  else /* direct compare */
    retval &= !memcmp(ent->test, buf, ent->test_len);

  if(ent->mask != 0xFFFFFFFF) {
    switch(ent->test_len) {
    case 1:
      retval &= ((ent->mask & ent->test[0]) == ent->mask);
      break;
    case 2:
      retval &= ((ent->mask & (*(guint16 *)ent->test)) == ent->mask);
      break;
    case 4:
      retval &= ((ent->mask & (*(guint32 *)ent->test)) == ent->mask);
      break;
    }
  }

  return retval;
}

static GnomeMagicEntry *
gnome_magic_db_load(void)
{
  int fd;
  char *filename;
  GnomeMagicEntry *retval;
  struct stat sbuf;

  filename = gnome_config_file("mime-magic.dat");

  if(!filename) return NULL;
  fd = open(filename, O_RDONLY);
  g_free(filename);
  if(fd < 0) return NULL;

  fstat(fd, &sbuf);

  retval = mmap(NULL, sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);

  close(fd);

  return retval;
}

const char *gnome_mime_type_from_magic(const char *filename)
{
  FILE *fh;
  static GnomeMagicEntry *ents = NULL;
  static char *fn;
  int i;
  struct stat sbuf;

  /* we really don't want to start reading from devices :) */
  stat(filename, &sbuf);
  if(!S_ISREG(sbuf.st_mode)) {
    if(S_ISDIR(sbuf.st_mode))
      return "special/directory";
    else if(S_ISCHR(sbuf.st_mode))
      return "special/device-char";
    else if(S_ISBLK(sbuf.st_mode))
      return "special/device-block";
    else if(S_ISFIFO(sbuf.st_mode))
      return "special/fifo";
    else if(S_ISSOCK(sbuf.st_mode))
      return "special/socket";
    else
      return NULL;
  }

  fh = fopen(filename, "r");
  if(!fh) return NULL;

  if(!ents) ents = gnome_magic_db_load();
  if(!ents) {
    char *fn = gnome_config_file("mime-magic");
    if(fn)
      ents = gnome_magic_parse(fn, NULL);
    g_free(fn);
  }
  if(!ents) return NULL;

  for(i = 0; ents[i].type != T_END; i++) {
    if(gnome_magic_matches_p(fh, &ents[i]))
      break;
  }

  fclose(fh);

  return (ents[i].type == T_END)?NULL:ents[i].mimetype;
}

#endif
