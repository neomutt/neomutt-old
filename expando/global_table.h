#ifndef EXPANDO_GLOBAL_TABLE_H
#define EXPANDO_GLOBAL_TABLE_H

#include "parser.h"

struct ExpandoRecord
{
  const char *string;
  struct ExpandoNode *tree;
};

struct ExpandoTable
{
  struct ExpandoRecord index_format;
};

#endif