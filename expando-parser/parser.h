#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stdio.h>

enum NodeType
{
  NT_TEXT,
  NT_EXPANDO,
  NT_DATE,
  NT_PAD,
  NT_CONDITION,
  NT_INDEX_FORMAT_HOOK
};

struct Node
{
  enum NodeType type;
  struct Node *next;
};

struct TextNode
{
  enum NodeType type;
  struct Node *next;

  const char *start;
  const char *end;
};

enum Justification
{
  FMT_J_RIGHT = 0,
  FMT_J_LEFT
};

struct Format
{
  int min;
  int max;
  enum Justification justification;
  char leader;

  const char *start;
  const char *end;
};

struct ExpandoNode
{
  enum NodeType type;
  struct Node *next;

  // can be used either fo %n or {name}
  const char *start;
  const char *end;

  const struct Format *format;
};

enum DateType
{
  DT_SENDER_SEND_TIME,
  DT_LOCAL_SEND_TIME,
  DT_LOCAL_RECIEVE_TIME,
};

struct DateNode
{
  enum NodeType type;
  struct Node *next;

  const char *start;
  const char *end;

  enum DateType date_type;
  bool ingnore_locale;
};

enum PadType
{
  PT_FILL,
  PT_HARD_FILL,
  PT_SOFT_FILL
};

struct PadNode
{
  enum NodeType type;
  struct Node *next;

  enum PadType pad_type;
  char pad_char;
};

enum ConditionStart
{
  CON_NO_CONDITION,
  CON_START
};

struct ConditionNode
{
  enum NodeType type;
  struct Node *next;

  struct Node *condition;
  struct Node *if_true;
  struct Node *if_false;
};

struct IndexFormatHookNode
{
  enum NodeType type;
  struct Node *next;

  const char *start;
  const char *end;
};

void parse_tree(struct Node **root, const char *s);
void free_tree(struct Node **root);
void print_tree(FILE *fp, struct Node **root);

#endif