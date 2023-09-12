/**
 * @file
 * XXX
 *
 * @authors
 * Copyright (C) 2023 Tóth János <gomba007@gmail.com>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MUTT_EXPANDO_NODE_H
#define MUTT_EXPANDO_NODE_H

#include <stdbool.h>
#include "gui/lib.h"
#include "format_callbacks.h"

enum ExpandoNodeType
{
  ENT_EMPTY = 0,
  ENT_TEXT,
  ENT_EXPANDO,
  ENT_DATE,
  ENT_PAD,
  ENT_CONDITION,
  ENT_INDEX_FORMAT_HOOK
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
  EDT_SENDER_SEND_TIME,
  EDT_LOCAL_SEND_TIME,
  EDT_LOCAL_RECIEVE_TIME,
};

struct ExpandoDatePrivate
{
  enum ExpandoDateType date_type;
  bool use_c_locale;
};

enum ExpandoPadType
{
  EPT_FILL_EOL,
  EPT_HARD_FILL,
  EPT_SOFT_FILL
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

#endif /* MUTT_EXPANDO_NODE_H */
