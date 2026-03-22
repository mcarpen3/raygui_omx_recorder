#include "utl_file.h"


static int sort_mp4s(const void *a, const void *b);
char *extname(char *filename)
{
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    if (strrchr(dot, '/') != NULL) return "";
    return dot + 1;
}

char **GetMp4s(int *count, char *path)
{
    int max_dir_files;
    *count = 0;

    char **videos = GetDirectoryFiles(path, &max_dir_files);
    char **mp4Files = NULL;
    for (int i = 0; i < max_dir_files; ++i)
    {
        if (!strcmp(extname(videos[i]), "mp4"))
        {
            (*count)++;
            mp4Files = realloc(mp4Files, ((*count) * sizeof(char *)));
            mp4Files[(*count) - 1] = videos[i];
        }
    }
    qsort(mp4Files, (size_t)*count, sizeof(char *), sort_mp4s);
    return mp4Files;
}

char *make_message(const char *fmt, ...)
{
   int size = 0;
   char *p = NULL;
   va_list ap;

   /* Determine required size */

   va_start(ap, fmt);
   size = vsnprintf(p, size, fmt, ap);
   va_end(ap);

   if (size < 0)
       return NULL;

   size++;             /* For '\0' */
   p = malloc(size);
   if (p == NULL)
       return NULL;

   va_start(ap, fmt);
   size = vsnprintf(p, size, fmt, ap);
   va_end(ap);

   if (size < 0) {
       free(p);
       return NULL;
   }

   return p;
}

static int sort_mp4s(const void *a, const void *b)
{
    // sort in descending order
    return -strcmp(* (char * const *) a, * (char * const *) b);
}

