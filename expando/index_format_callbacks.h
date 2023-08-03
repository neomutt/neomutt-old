#ifndef EXPANDO_INDEX_FORMAT_CALLBACKS_H
#define EXPANDO_INDEX_FORMAT_CALLBACKS_H

#include "format_callbacks.h"

void index_C(const struct ExpandoNode *self, char **buffer, int *buffer_len,
             int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags);
#endif