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

/**
 * @page expando_parser XXX
 *
 * XXX
 */

#include "config.h"
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mutt/memory.h"
#include "gui/curs_lib.h"
#include "lib.h"
#include "limits.h"

extern const struct ExpandoValidation expando_validation[EFMTI_FORMAT_COUNT_OR_DEBUG];

/**
 * ExpandoConditionStart - Signals parse_node() if the parsing started in a conditional statement or not
 *
 * Easier to read than a simple true, false.
 */
enum ExpandoConditionStart
{
  CON_NO_CONDITION,
  CON_START
};

static struct ExpandoNode *new_empty_node(void)
{
  struct ExpandoNode *node = mutt_mem_calloc(1, sizeof(struct ExpandoNode));

  node->type = ENT_EMPTY;

  return node;
}

static struct ExpandoNode *new_text_node(const char *start, const char *end)
{
  struct ExpandoNode *node = mutt_mem_calloc(1, sizeof(struct ExpandoNode));

  node->type = ENT_TEXT;
  node->start = start;
  node->end = end;

  node->format_cb = text_format_callback;

  return node;
}

static struct ExpandoNode *new_expando_node(const char *start, const char *end,
                                            const struct ExpandoFormatPrivate *format,
                                            expando_format_callback cb)
{
  struct ExpandoNode *node = mutt_mem_calloc(1, sizeof(struct ExpandoNode));

  node->type = ENT_EXPANDO;
  node->start = start;
  node->end = end;

  node->ndata = (void *) format;
  node->ndata_free = free_expando_private;

  node->format_cb = cb;

  return node;
}

static struct ExpandoNode *new_date_node(const char *start, const char *end,
                                         enum ExpandoDateType date_type,
                                         bool use_c_locale, expando_format_callback cb)
{
  struct ExpandoNode *node = mutt_mem_calloc(1, sizeof(struct ExpandoNode));

  node->type = ENT_DATE;
  node->start = start;
  node->end = end;

  node->format_cb = cb;

  struct ExpandoDatePrivate *dp = mutt_mem_calloc(1, sizeof(struct ExpandoDatePrivate));
  dp->date_type = date_type;
  dp->use_c_locale = use_c_locale;

  node->ndata = (void *) dp;
  node->ndata_free = free_expando_private;

  return node;
}

static struct ExpandoNode *new_pad_node(enum ExpandoPadType pad_type,
                                        const char *start, const char *end)
{
  struct ExpandoNode *node = mutt_mem_calloc(1, sizeof(struct ExpandoNode));

  node->type = ENT_PAD;
  node->start = start;
  node->end = end;

  node->format_cb = pad_format_callback;

  struct ExpandoPadPrivate *pp = mutt_mem_calloc(1, sizeof(struct ExpandoPadPrivate));
  pp->pad_type = pad_type;

  node->ndata = (void *) pp;
  node->ndata_free = free_expando_private;

  return node;
}

static struct ExpandoNode *new_condition_node(struct ExpandoNode *condition,
                                              struct ExpandoNode *if_true_tree,
                                              struct ExpandoNode *if_false_tree)
{
  assert(condition != NULL);
  assert(if_true_tree != NULL);

  struct ExpandoNode *node = mutt_mem_calloc(1, sizeof(struct ExpandoNode));

  node->type = ENT_CONDITION;
  node->format_cb = conditional_format_callback;

  struct ExpandoConditionPrivate *cp = mutt_mem_calloc(1, sizeof(struct ExpandoConditionPrivate));
  cp->condition = condition;
  cp->if_true_tree = if_true_tree;
  cp->if_false_tree = if_false_tree;

  node->ndata = (void *) cp;
  node->ndata_free = free_expando_private_condition_node;

  return node;
}

static struct ExpandoNode *new_index_format_hook_node(const char *start, const char *end)
{
  struct ExpandoNode *node = mutt_mem_calloc(1, sizeof(struct ExpandoNode));

  node->type = ENT_INDEX_FORMAT_HOOK;
  node->start = start;
  node->end = end;
  node->format_cb = index_format_hook_callback;

  return node;
}

static void append_node(struct ExpandoNode **root, struct ExpandoNode *new_node)
{
  if (!*root)
  {
    *root = new_node;
    return;
  }

  struct ExpandoNode *n = *root;
  while (n->next != NULL)
  {
    n = n->next;
  }

  n->next = new_node;
}

static bool is_valid_classic_expando(char c)
{
  // $sidebar_format
  return isalpha(c) || c == '!';
}

static const char *skip_until_classic_expando(const char *start)
{
  while (*start && !is_valid_classic_expando(*start))
  {
    ++start;
  }

  return start;
}

// NOTE(g0mb4): no multibyte classic expando is allowed
static const char *skip_classic_expando(const char *s, enum ExpandoFormatIndex index)
{
  const struct ExpandoFormatCallback *valid_two_char_expandos = NULL;

  if (index >= 0 && index < EFMTI_FORMAT_COUNT_OR_DEBUG)
  {
    valid_two_char_expandos = expando_validation[index].valid_two_char_expandos;
  }

  for (size_t i = 0; valid_two_char_expandos && valid_two_char_expandos[i].name != NULL; ++i)
  {
    if (valid_two_char_expandos[i].name[0] == *s &&
        valid_two_char_expandos[i].name[1] == *(s + 1))
    {
      s++;
      break;
    }
  }

  s++;
  return s;
}

// NOTE(g0mb4): Not used, but keep it for now.
const char *skip_until_chrs(const char *start, const char *terminators)
{
  while (*start && strchr(terminators, *start) == NULL)
  {
    ++start;
  }

  return start;
}

static const char *skip_until_ch(const char *start, char terminator)
{
  while (*start && *start != terminator)
  {
    ++start;
  }

  return start;
}

static const char *skip_until_ch2(const char *start, char terminator1, char terminator2)
{
  while (*start && !(*start == terminator1 || *start == terminator2))
  {
    ++start;
  }

  return start;
}

static const struct ExpandoFormatPrivate *
parse_format(const char *start, const char *end, struct ExpandoParseError *error)
{
  if (start == end)
  {
    return NULL;
  }

  struct ExpandoFormatPrivate *format = mutt_mem_calloc(1, sizeof(struct ExpandoFormatPrivate));

  format->leader = ' ';
  format->start = start;
  format->end = end;
  format->justification = JUSTIFY_RIGHT;
  format->min = 0;
  format->max = INT_MAX;

  bool is_min = true;

  while (start < end)
  {
    switch (*start)
    {
      case '-':
        format->justification = JUSTIFY_LEFT;
        ++start;
        break;

      case '=':
        format->justification = JUSTIFY_CENTER;
        ++start;
        break;

      // NOTE(gmb): multibyte leader?
      case '0':
        format->leader = '0';
        ++start;
        break;

      case '.':
      {
        if (!isdigit(*(start + 1)))
        {
          error->position = start + 1;
          snprintf(error->message, sizeof(error->message), "Wrong number.");
          return NULL;
        }
        is_min = false;
        ++start;
      }

      break;

      // number
      default:
      {
        if (!isdigit(*start))
        {
          error->position = start;
          snprintf(error->message, sizeof(error->message), "Wrong number.");
          return NULL;
        }

        char *end_ptr;
        int number = strtol(start, &end_ptr, 10);

        // NOTE: start is NOT null-terminated
        if (end_ptr > end)
        {
          error->position = start;
          snprintf(error->message, sizeof(error->message), "Wrong number.");
          return NULL;
        }

        if (is_min)
        {
          format->min = number;
        }
        else
        {
          format->max = number;
        }

        start = end_ptr;
      }
    }
  }

  return format;
}

static expando_format_callback check_if_expando_is_valid(const char *start, const char *end,
                                                         enum ExpandoFormatIndex index,
                                                         struct ExpandoParseError *error)
{
  const struct ExpandoFormatCallback *valid_short_expandos = NULL;
  const struct ExpandoFormatCallback *valid_two_char_expandos = NULL;
  const struct ExpandoFormatCallback *valid_long_expandos = NULL;

  if (index >= 0 && index < EFMTI_FORMAT_COUNT_OR_DEBUG)
  {
    valid_short_expandos = expando_validation[index].valid_short_expandos;
    valid_two_char_expandos = expando_validation[index].valid_two_char_expandos;
    valid_long_expandos = expando_validation[index].valid_long_expandos;
  }

  if (valid_short_expandos && mb_strlen_nonnull(start, end) == 1)
  {
    for (size_t i = 0; valid_short_expandos[i].name != NULL; ++i)
    {
      const int len = strlen(valid_short_expandos[i].name);
      if (strncmp(start, valid_short_expandos[i].name, len) == 0)
      {
        // TODO(g0mb4): Activate assert as soon as the global table is filled.
        // assert(valid_short_expandos[i].callback);
        return valid_short_expandos[i].callback;
      }
    }

    error->position = start;
    const int len = end - start;
    snprintf(error->message, sizeof(error->message), "Invalid expando: `%.*s`.", len, start);
    return NULL;
  }

  if (valid_two_char_expandos && mb_strlen_nonnull(start, end) > 1)
  {
    for (size_t i = 0; valid_two_char_expandos[i].name != NULL; ++i)
    {
      const int len = strlen(valid_two_char_expandos[i].name);
      if (strncmp(start, valid_two_char_expandos[i].name, len) == 0)
      {
        // TODO(g0mb4): Activate assert as soon as the global table is filled.
        // assert(valid_two_char_expandos[i].callback);
        return valid_two_char_expandos[i].callback;
      }
    }

    error->position = start;
    const int len = end - start;
    snprintf(error->message, sizeof(error->message), "Invalid expando: `%.*s`.", len, start);
    return NULL;
  }

  if (valid_long_expandos && mb_strlen_nonnull(start, end) > 1)
  {
    for (size_t i = 0; valid_long_expandos[i].name != NULL; ++i)
    {
      const int len = strlen(valid_long_expandos[i].name);
      if (strncmp(start, valid_long_expandos[i].name, len) == 0)
      {
        // TODO(g0mb4): Activate assert as soon as the global table is filled.
        // assert(valid_long_expandos[i].callback);
        return valid_long_expandos[i].callback;
      }
    }

    error->position = start;
    const int len = end - start;
    snprintf(error->message, sizeof(error->message), "Invalid expando: `%.*s`.", len, start);
    return NULL;
  }

  return NULL;
}

static struct ExpandoNode *parse_node(const char *s, enum ExpandoConditionStart condition_start,
                                      const char **parsed_until, char terminator,
                                      enum ExpandoFormatIndex index,
                                      struct ExpandoParseError *error)
{
  while (*s)
  {
    if (*s == '%' || (condition_start == CON_START && (*s == '?' || *s == '<')))
    {
      s++;
      // dates
      if (*s == '{' || *s == '[' || *s == '(')
      {
        if (!(index == EFMTI_FORMAT_COUNT_OR_DEBUG || index == EFMTI_INDEX_FORMAT ||
              index == EFMTI_PGP_ENTRY_FORMAT || index == EFMTI_FOLDER_FORMAT ||
              index == EFMTI_MAILBOX_FOLDER_FORMAT))
        {
          error->position = s;
          snprintf(error->message, sizeof(error->message), "Date is not allowed.");
          return NULL;
        }

        // TODO(g0mb4): handle {name} expandos!
        expando_format_callback date_cb = NULL;
        bool ignore_locale = false;
        const char *start = s + 1;
        if (*(s + 1) == '!')
        {
          ignore_locale = true;
          start++;
        }

        enum ExpandoDateType dt = 0;
        const char *end = NULL;

        if (*s == '{')
        {
          if (index == EFMTI_PGP_ENTRY_FORMAT || index == EFMTI_FOLDER_FORMAT ||
              index == EFMTI_MAILBOX_FOLDER_FORMAT)
          {
            error->position = s;
            snprintf(error->message, sizeof(error->message), "Invalid date format.");
            return NULL;
          }

          ++s;
          dt = EDT_SENDER_SEND_TIME;
          end = skip_until_ch(s, '}');
          if (*end != '}')
          {
            error->position = end;
            snprintf(error->message, sizeof(error->message), "Missing '}'.");
            return NULL;
          }
        }
        else if (*s == '[')
        {
          ++s;
          dt = EDT_LOCAL_SEND_TIME;
          end = skip_until_ch(s, ']');
          if (*end != ']')
          {
            error->position = end;
            snprintf(error->message, sizeof(error->message), "Missing ']'.");
            return NULL;
          }
        }
        // '('
        else
        {
          if (index == EFMTI_PGP_ENTRY_FORMAT || index == EFMTI_FOLDER_FORMAT ||
              index == EFMTI_MAILBOX_FOLDER_FORMAT)
          {
            error->position = s;
            snprintf(error->message, sizeof(error->message), "Invalid date format.");
            return NULL;
          }

          ++s;
          dt = EDT_LOCAL_RECIEVE_TIME;
          end = skip_until_ch(s, ')');
          if (*end != ')')
          {
            error->position = end;
            snprintf(error->message, sizeof(error->message), "Missing ')'.");
            return NULL;
          }
        }

        if (index == EFMTI_INDEX_FORMAT)
        {
          date_cb = index_date;
        }
        else if (index == EFMTI_PGP_ENTRY_FORMAT)
        {
          date_cb = pgp_entry_date;
        }
        else if (index == EFMTI_FOLDER_FORMAT || index == EFMTI_MAILBOX_FOLDER_FORMAT)
        {
          date_cb = folder_date;
        }
        else
        {
          /* do nothing */
        }

        *parsed_until = end + 1;
        return new_date_node(start, end, dt, ignore_locale, date_cb);
      }
      // padding
      else if (*s == '|' || *s == '>' || *s == '*')
      {
        enum ExpandoPadType pt = 0;
        if (*s == '|')
        {
          pt = EPT_FILL_EOL;
        }
        else if (*s == '>')
        {
          pt = EPT_HARD_FILL;
        }
        // '*'s
        else
        {
          pt = EPT_SOFT_FILL;
        }
        s++;

        int consumed = mbtowc(NULL, s, MB_CUR_MAX);
        if (consumed <= 0)
        {
          error->position = s;
          snprintf(error->message, sizeof(error->message), "Invalid padding character.");
          return NULL;
        }

        *parsed_until = s + consumed;
        return new_pad_node(pt, s, s + consumed);
      }
      // "%"
      else if (*s == '%')
      {
        *parsed_until = s + 1;
        return new_text_node(s, s + 1);
      }
      // conditional
      else if (*s == '?' || *s == '<')
      {
        bool old_style = *s == '?';
        char end_terminator = old_style ? '?' : '>';

        const char *next = NULL;
        struct ExpandoNode *condition = parse_node(s, CON_START, &next,
                                                   terminator, index, error);
        if (condition == NULL)
        {
          return NULL;
        }

        if (*next != '?')
        {
          error->position = next;
          snprintf(error->message, sizeof(error->message), "Missing '?'.");
          return NULL;
        }

        s = next + 1;

        const char *if_true_end = skip_until_ch2(s, '&', end_terminator);

        bool only_true = *if_true_end == end_terminator;
        bool invalid = *if_true_end != '&' && !only_true;

        if (invalid)
        {
          error->position = if_true_end;
          snprintf(error->message, sizeof(error->message),
                   "Missing '&' or '%c'.", end_terminator);
          return NULL;
        }

        const char *if_true_start = s;
        const char *if_true_parsed = NULL;
        struct ExpandoNode *if_true_tree = NULL;

        while (if_true_start < if_true_end)
        {
          char if_terminator = only_true ? end_terminator : '&';

          struct ExpandoNode *n = parse_node(if_true_start, CON_NO_CONDITION, &if_true_parsed,
                                             if_terminator, index, error);
          if (n == NULL)
          {
            return NULL;
          }

          append_node(&if_true_tree, n);

          if_true_start = if_true_parsed;
        }

        if (only_true)
        {
          *parsed_until = if_true_end + 1;
          return new_condition_node(condition, if_true_tree, NULL);
        }
        else
        {
          const char *if_false_start = if_true_end + 1;
          const char *if_false_end = skip_until_ch(s, end_terminator);

          if (*if_false_end != end_terminator)
          {
            error->position = s;
            snprintf(error->message, sizeof(error->message), "Missing '%c'.", end_terminator);
            return NULL;
          }

          const char *if_false_parsed = NULL;
          struct ExpandoNode *if_false_tree = NULL;

          while (if_false_start < if_false_end)
          {
            struct ExpandoNode *n = parse_node(if_false_start, CON_NO_CONDITION, &if_false_parsed,
                                               end_terminator, index, error);
            if (n == NULL)
            {
              return NULL;
            }

            append_node(&if_false_tree, n);

            if_false_start = if_false_parsed;
          }

          *parsed_until = if_false_end + 1;
          return new_condition_node(condition, if_true_tree, if_false_tree);
        }
      }
      // index format hook
      else if (*s == '@')
      {
        if (!(index == EFMTI_FORMAT_COUNT_OR_DEBUG || index == EFMTI_INDEX_FORMAT))
        {
          error->position = s;
          snprintf(error->message, sizeof(error->message), "Index-hook-format is not allowed.");
          return NULL;
        }

        ++s;
        const char *end = skip_until_ch(s, '@');

        if (*end != '@')
        {
          error->position = end;
          snprintf(error->message, sizeof(error->message), "Missing '@'.");
          return NULL;
        }

        *parsed_until = end + 1;
        return new_index_format_hook_node(s, end);
      }
      // classic expandos: %X
      else
      {
        const char *format_end = skip_until_classic_expando(s);
        const char *expando_end = skip_classic_expando(format_end, index);
        const struct ExpandoFormatPrivate *format = parse_format(s, format_end, error);
        if (error->position != NULL)
        {
          return NULL;
        }

        expando_format_callback callback = check_if_expando_is_valid(format_end, expando_end,
                                                                     index, error);
        if (error->position != NULL)
        {
          return NULL;
        }

        *parsed_until = expando_end;
        return new_expando_node(format_end, expando_end, format, callback);
      }
    }
    // text
    else
    {
      const char *end = skip_until_ch2(s, '%', terminator);
      *parsed_until = end;
      return new_text_node(s, end);
    }
  }

  error->position = s;
  snprintf(error->message, sizeof(error->message), "Internal parsing error.");
  return NULL;
}

void expando_tree_parse(struct ExpandoNode **root, const char **string,
                        enum ExpandoFormatIndex index, struct ExpandoParseError *error)
{
  if (!string || !*string || !**string)
  {
    append_node(root, new_empty_node());
    return;
  }

  const char *end = NULL;
  const char *start = *string;

  while (*start)
  {
    struct ExpandoNode *n = parse_node(start, CON_NO_CONDITION, &end, 0, index, error);

    if (n == NULL)
    {
      break;
    }

    append_node(root, n);
    start = end;
  }
}

void expando_tree_free(struct ExpandoNode **root)
{
  struct ExpandoNode *n = *root;
  free_tree(n);
}
