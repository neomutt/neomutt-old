#ifndef EXPANDO_INDEX_FORMAT_CALLBACKS_H
#define EXPANDO_INDEX_FORMAT_CALLBACKS_H

#include "format_callbacks.h"

int index_C(const struct ExpandoNode *self, char *buf, int buflen, int col,
            int cols, intptr_t data, MuttFormatFlags flags);
#endif