#include "utl_file.h"

char *extname(char *filename)
{
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    if (strrchr(dot, '/') != NULL) return "";
    return dot + 1;
}

char **GetMp4s(int *count)
{
    int max_dir_files;
    char *path;
    *count = 0;
#ifndef OUTPUT_VID_DIR
    path = "OutputVideos";
#else
    path = OUTPUT_VID_DIR;
#endif
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
    return mp4Files;
}

