#ifndef EXPANDO_FORMAT_CALLBACKS_H
#define EXPANDO_FORMAT_CALLBACKS_H

#include "format_flags.h"

struct ExpandoNode;

typedef void (*expando_format_callback)(const struct ExpandoNode *self, char **buffer,
                                        int *buffer_len, int *start_col, int max_cols,
                                        intptr_t data, MuttFormatFlags flags);

void format_tree(struct ExpandoNode **tree, char *buf, size_t buflen, size_t col, int cols,
                 intptr_t data, MuttFormatFlags flags);

void text_format_callback(const struct ExpandoNode *self, char **buffer,
                          int *buffer_len, int *start_col, int max_cols,
                          intptr_t data, MuttFormatFlags flags);

void conditional_format_callback(const struct ExpandoNode *self, char **buffer,
                                 int *buffer_len, int *start_col, int max_cols,
                                 intptr_t data, MuttFormatFlags flags);

#endif