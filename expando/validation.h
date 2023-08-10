#ifndef MUTT_EXPANDO_VALIDATION_H
#define MUTT_EXPANDO_VALIDATION_H

#include <stdbool.h>
#include "format_callbacks.h"

struct Buffer;

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

#endif /* MUTT_EXPANDO_VALIDATION_H */
