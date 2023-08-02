#ifndef EXPANDO_PARSER_H
#define EXPANDO_PARSER_H

#include <stdbool.h>
#include <stdio.h>
#include "format_callbacks.h"

enum ExpandoNodeType
{
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
  format_callback format_cb;
};

struct ExpandoTextNode
{
  enum ExpandoNodeType type;
  struct ExpandoNode *next;
  format_callback format_cb;

  const char *start;
  const char *end;
};

enum ExpandoFormatJustification
{
  FMT_J_RIGHT = 0,
  FMT_J_LEFT
};

struct ExpandoFormat
{
  int min;
  int max;
  enum ExpandoFormatJustification justification;
  // NOTE(gmb): multibyte leader?
  char leader;

  const char *start;
  const char *end;
};

struct ExpandoExpandoNode
{
  enum ExpandoNodeType type;
  struct ExpandoNode *next;
  format_callback format_cb;

  // can be used either fo %n or {name}
  const char *start;
  const char *end;

  const struct ExpandoFormat *format;
};

enum ExpandoDateType
{
  DT_SENDER_SEND_TIME,
  DT_LOCAL_SEND_TIME,
  DT_LOCAL_RECIEVE_TIME,
};

struct ExpandoDateNode
{
  enum ExpandoNodeType type;
  struct ExpandoNode *next;
  format_callback format_cb;

  const char *start;
  const char *end;

  enum ExpandoDateType date_type;
  bool ingnore_locale;
};

enum ExpandoPadType
{
  PT_FILL,
  PT_HARD_FILL,
  PT_SOFT_FILL
};

struct ExpandoPadNode
{
  enum ExpandoNodeType type;
  struct ExpandoNode *next;
  format_callback format_cb;

  enum ExpandoPadType pad_type;
  const char *start;
  const char *end;
};

enum ExpandoConditionStart
{
  CON_NO_CONDITION,
  CON_START
};

struct ExpandoConditionNode
{
  enum ExpandoNodeType type;
  struct ExpandoNode *next;
  format_callback format_cb;

  struct ExpandoNode *condition;
  struct ExpandoNode *if_true;
  struct ExpandoNode *if_false;
};

struct ExpandoIndexFormatHookNode
{
  enum ExpandoNodeType type;
  struct ExpandoNode *next;
  format_callback format_cb;

  const char *start;
  const char *end;
};

struct ExpandoParseError
{
  char message[128];
  const char *position;
};

void expando_tree_parse(struct ExpandoNode **root, const char **string,
                        const char *valid_short_expandos[],
                        const char *valid_two_char_expandos[],
                        const char *valid_long_expandos[], struct ExpandoParseError *error);
void expando_tree_free(struct ExpandoNode **root);
void expando_tree_print(FILE *fp, struct ExpandoNode **root);

#endif
