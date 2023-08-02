#ifndef EXPANDO_VALIDATION_H
#define EXPANDO_VALIDATION_H

#include "mutt/buffer.h"
#include "format_callbacks.h"
#include "global_table.h"
#include "parser.h"

struct ExpandoFormatCallback
{
  const char *name;
  expando_format_callback callback;
};

struct ExpandoValidation
{
  const char *name;
  const struct ExpandoFormatCallback *valid_short_expandos;
  const struct ExpandoFormatCallback *valid_two_char_expandos;
  // TODO(g0mb4): add valid_long_expandos
};

bool expando_validate_string(struct Buffer *name, struct Buffer *value, struct Buffer *err);

#endif