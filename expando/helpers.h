#ifndef EXPANDO_HELPERS_H
#define EXPANDO_HELPERS_H

#include <time.h>

int mb_strlen_range(const char *start, const char *end);
int mb_strwidth_range(const char *start, const char *end);

void strftime_range(char *s, size_t max, const char *format_start,
                    const char *format_end, const struct tm *tm);
#endif