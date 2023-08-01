#ifndef EXPANDO_GLOBAL_TABLE_H
#define EXPANDO_GLOBAL_TABLE_H

#include "parser.h"

struct ExpandoRecord
{
  const char *string;
  struct ExpandoNode *tree;
};

enum ExpandoFormatIndex
{
  EFMT_ALIAS_FORMAT = 0,
  EFMT_ATTACH_FORMAT,
  EFMT_INDEX_FORMAT,
  EFMT_FORMAT_COUNT,
};

#endif