#ifndef MUTT_EXPANDO_PARSER_H
#define MUTT_EXPANDO_PARSER_H

#include <stdio.h>
#include "global_table.h"

struct ExpandoNode;

struct ExpandoParseError
{
  char message[128];
  const char *position;
};

void expando_tree_parse(struct ExpandoNode **root, const char **string,
                        enum ExpandoFormatIndex index, struct ExpandoParseError *error);
void expando_tree_free(struct ExpandoNode **root);
void expando_tree_print(FILE *fp, struct ExpandoNode **root, int indent);

#endif /* MUTT_EXPANDO_PARSER_H */
