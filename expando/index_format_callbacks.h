#ifndef MUTT_EXPANDO_INDEX_FORMAT_CALLBACKS_H
#define MUTT_EXPANDO_INDEX_FORMAT_CALLBACKS_H

#include <stdint.h>
#include "format_flags.h"

struct ExpandoNode;

void index_date(const struct ExpandoNode *self, char **buffer, int *buffer_len,
                int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags);

void index_C(const struct ExpandoNode *self, char **buffer, int *buffer_len,
             int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags);

void index_Z(const struct ExpandoNode *self, char **buffer, int *buffer_len,
             int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags);

void index_L(const struct ExpandoNode *self, char **buffer, int *buffer_len,
             int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags);

void index_s(const struct ExpandoNode *self, char **buffer, int *buffer_len,
             int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags);

void index_l(const struct ExpandoNode *self, char **buffer, int *buffer_len,
             int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags);

void index_c(const struct ExpandoNode *self, char **buffer, int *buffer_len,
             int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags);

void index_cr(const struct ExpandoNode *self, char **buffer, int *buffer_len,
              int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags);

#endif /* MUTT_EXPANDO_INDEX_FORMAT_CALLBACKS_H */
