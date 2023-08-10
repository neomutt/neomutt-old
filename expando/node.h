#ifndef EXPANDO_NODE_H
#define EXPANDO_NODE_H

#include <stdbool.h>
#include "gui/curs_lib.h"
#include "format_callbacks.h"

enum ExpandoNodeType
{
  NT_EMPTY = 0,
  NT_TEXT,
  NT_EXPANDO,
  NT_DATE,
  NT_PAD,
  NT_CONDITION,
  NT_INDEX_FORMAT_HOOK
};

struct ExpandoNode
{
  enum ExpandoNodeType type;
  struct ExpandoNode *next;
  expando_format_callback format_cb;

  const char *start;
  const char *end;

  void *ndata;
  void (*ndata_free)(void **ptr);
};

struct ExpandoFormatPrivate
{
  int min;
  int max;
  enum FormatJustify justification;
  // NOTE(gmb): multibyte leader?
  char leader;

  const char *start;
  const char *end;
};

enum ExpandoDateType
{
  DT_SENDER_SEND_TIME,
  DT_LOCAL_SEND_TIME,
  DT_LOCAL_RECIEVE_TIME,
};

struct ExpandoDatePrivate
{
  enum ExpandoDateType date_type;
  bool ingnore_locale;
};

enum ExpandoPadType
{
  PT_FILL,
  PT_HARD_FILL,
  PT_SOFT_FILL
};

struct ExpandoPadPrivate
{
  enum ExpandoPadType pad_type;
};

struct ExpandoConditionPrivate
{
  struct ExpandoNode *condition;
  struct ExpandoNode *if_true_tree;
  struct ExpandoNode *if_false_tree;
};

void free_node(struct ExpandoNode *node);
void free_tree(struct ExpandoNode *node);

void free_expando_private(void **ptr);
void free_expando_private_condition_node(void **ptr);

#endif