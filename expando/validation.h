#ifndef EXPANDO_VALIDATION_H
#define EXPANDO_VALIDATION_H

#include "mutt/buffer.h"
#include "global_table.h"
#include "parser.h"

struct ExpandoValidation
{
  const char *name;
  const char **valid_short_expandos;
  const char **valid_two_char_expandos;
  // TODO(g0mb4): add valid_long_expandos
};

bool expando_validate_string(struct Buffer *name, struct Buffer *value, struct Buffer *err);

#endif