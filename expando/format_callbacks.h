#ifndef EXPANDO_FORMAT_CALLBACKS_H
#define EXPANDO_FORMAT_CALLBACKS_H

#include "format_flags.h"

struct ExpandoNode;

typedef int (*expando_format_callback)(const struct ExpandoNode *self,
                                       char *buf, int buflen, int col, int cols,
                                       intptr_t data, MuttFormatFlags flags);

int text_format_callback(const struct ExpandoNode *self, char *buf, int buflen,
                         int col, int cols, intptr_t data, MuttFormatFlags flags);

#endif