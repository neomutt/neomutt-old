#ifndef EXPANDO_PARSER_H
#define EXPANDO_PARSER_H

#include <stdbool.h>
#include <stdio.h>

#include "format_callbacks.h"
#include "global_table.h"
#include "node.h"

struct ExpandoParseError
{
  char message[128];
  const char *position;
};

struct ExpandoNode;
struct ExpandoFormatCallback;

void expando_tree_parse(struct ExpandoNode **root, const char **string,
                        enum ExpandoFormatIndex index, struct ExpandoParseError *error);
void expando_tree_free(struct ExpandoNode **root);
void expando_tree_print(FILE *fp, struct ExpandoNode **root, int indent);

#endif
