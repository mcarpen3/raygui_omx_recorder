#ifndef UTL_FILE_H_
#define UTL_FILE_H_
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "raylib.h"
char *extname(char *filename);
char **GetMp4s(int *count, char *path);
char *make_message(const char *fmt, ...);
#endif
