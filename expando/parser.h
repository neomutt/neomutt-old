#ifndef EXPANDO_PARSER_H
#define EXPANDO_PARSER_H

#include <stdbool.h>
#include <stdio.h>

#include "format_callbacks.h"
#include "node.h"

struct ExpandoParseError
{
  char message[128];
  const char *position;
};

struct ExpandoNode;
struct ExpandoFormatCallback;

void expando_tree_parse(struct ExpandoNode **root, const char **string,
                        const struct ExpandoFormatCallback *valid_short_expandos,
                        const struct ExpandoFormatCallback *valid_two_char_expandos,
                        const struct ExpandoFormatCallback *valid_long_expandos,
                        struct ExpandoParseError *error);
void expando_tree_free(struct ExpandoNode **root);
void expando_tree_print(FILE *fp, struct ExpandoNode **root);

#endif
