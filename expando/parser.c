/**
 * DISCLAIMER: This is a work-in-progress code.
 * 
 * Tóth János
 */

#include "format_callbacks.h"
#include "helpers.h"
#include "parser.h"
#include "validation.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "mutt/lib.h"
#include "mutt/memory.h"

static void free_node(struct ExpandoNode *n);
static void print_node(FILE *fp, const struct ExpandoNode *n, int indent);

static struct ExpandoNode *new_empty_node(void)
{
  struct ExpandoNode *node = mutt_mem_calloc(1, sizeof(struct ExpandoNode));

  node->type = NT_EMPTY;

  return node;
}

static struct ExpandoNode *new_text_node(const char *start, const char *end)
{
  struct ExpandoTextNode *node = mutt_mem_calloc(1, sizeof(struct ExpandoTextNode));

  node->type = NT_TEXT;
  node->start = start;
  node->end = end;

  node->format_cb = text_format_callback;

  return (struct ExpandoNode *) node;
}

static struct ExpandoNode *new_expando_node(const char *start, const char *end,
                                            const struct ExpandoFormat *format,
                                            expando_format_callback cb)
{
  struct ExpandoExpandoNode *node = mutt_mem_calloc(1, sizeof(struct ExpandoExpandoNode));

  node->type = NT_EXPANDO;
  node->start = start;
  node->end = end;
  node->format = format;
  node->format_cb = cb;

  return (struct ExpandoNode *) node;
}

static struct ExpandoNode *new_date_node(const char *start, const char *end,
                                         enum ExpandoDateType date_type, bool ingnore_locale)
{
  struct ExpandoDateNode *node = mutt_mem_calloc(1, sizeof(struct ExpandoDateNode));

  node->type = NT_DATE;
  node->start = start;
  node->end = end;
  node->date_type = date_type;
  node->ingnore_locale = ingnore_locale;

  return (struct ExpandoNode *) node;
}

static struct ExpandoNode *new_pad_node(enum ExpandoPadType pad_type,
                                        const char *start, const char *end)
{
  struct ExpandoPadNode *node = mutt_mem_calloc(1, sizeof(struct ExpandoPadNode));

  node->type = NT_PAD;
  node->pad_type = pad_type;
  node->start = start;
  node->end = end;

  return (struct ExpandoNode *) node;
}

static struct ExpandoNode *new_condition_node(struct ExpandoNode *condition,
                                              struct ExpandoNode *if_true,
                                              struct ExpandoNode *if_false)
{
  assert(condition != NULL);
  assert(if_true != NULL);

  struct ExpandoConditionNode *node = mutt_mem_calloc(1, sizeof(struct ExpandoConditionNode));

  node->type = NT_CONDITION;
  node->condition = condition;
  node->if_true = if_true;
  node->if_false = if_false;

  return (struct ExpandoNode *) node;
}

static struct ExpandoNode *new_index_format_hook_node(const char *start, const char *end)
{
  struct ExpandoIndexFormatHookNode *node = mutt_mem_calloc(1, sizeof(struct ExpandoIndexFormatHookNode));

  node->type = NT_INDEX_FORMAT_HOOK;
  node->start = start;
  node->end = end;

  return (struct ExpandoNode *) node;
}

static void free_empty_node(struct ExpandoNode *n)
{
  FREE(&n);
}

static void free_text_node(struct ExpandoTextNode *n)
{
  FREE(&n);
}

static void free_date_node(struct ExpandoDateNode *n)
{
  FREE(&n);
}

static void free_expando_node(struct ExpandoExpandoNode *n)
{
  if (n->format)
  {
    FREE(&n->format);
  }

  FREE(&n);
}

static void free_pad_node(struct ExpandoPadNode *n)
{
  FREE(&n);
}

static void free_condition_node(struct ExpandoConditionNode *n)
{
  if (n->condition)
  {
    free_node(n->condition);
  }

  if (n->if_true)
  {
    free_node(n->if_true);
  }

  if (n->if_false)
  {
    free_node(n->if_false);
  }

  FREE(&n);
}

static void free_index_format_hook_node(struct ExpandoIndexFormatHookNode *n)
{
  FREE(&n);
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
static const char *skip_classic_expando(const char *s, const struct ExpandoFormatCallback *valid_two_char_expandos)
{
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

static const char *skip_until(const char *start, const char *terminator)
{
  while (*start && strchr(terminator, *start) == NULL)
  {
    ++start;
  }

  return start;
}

static const struct ExpandoFormat *parse_format(const char *start, const char *end,
                                                struct ExpandoParseError *error)
{
  if (start == end)
  {
    return NULL;
  }

  struct ExpandoFormat *format = mutt_mem_calloc(1, sizeof(struct ExpandoFormat));

  format->leader = ' ';
  format->start = start;
  format->end = end;

  bool is_min = true;

  while (start < end)
  {
    switch (*start)
    {
      case '-':
        format->justification = FMT_J_LEFT;
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

static expando_format_callback
check_if_expando_is_valid(const char *start, const char *end,
                          const struct ExpandoFormatCallback *valid_short_expandos,
                          const struct ExpandoFormatCallback *valid_two_char_expandos,
                          const struct ExpandoFormatCallback *valid_long_expandos,
                          struct ExpandoParseError *error)
{
  if (valid_short_expandos && mb_strlen_range(start, end) == 1)
  {
    for (size_t i = 0; valid_short_expandos[i].name != NULL; ++i)
    {
      const int len = strlen(valid_short_expandos[i].name);
      if (strncmp(start, valid_short_expandos[i].name, len) == 0)
      {
        // TODO(g0mb4): enable assert
        // assert(valid_short_expandos[i].callback);
        return valid_short_expandos[i].callback;
      }
    }

    error->position = start;
    const int len = end - start;
    snprintf(error->message, sizeof(error->message), "Invalid expando: `%.*s`.", len, start);
    return NULL;
  }

  if (valid_two_char_expandos && mb_strlen_range(start, end) > 1)
  {
    for (size_t i = 0; valid_two_char_expandos[i].name != NULL; ++i)
    {
      const int len = strlen(valid_two_char_expandos[i].name);
      if (strncmp(start, valid_two_char_expandos[i].name, len) == 0)
      {
        return valid_two_char_expandos[i].callback;
      }
    }

    error->position = start;
    const int len = end - start;
    snprintf(error->message, sizeof(error->message), "Invalid expando: `%.*s`.", len, start);
    return NULL;
  }

  if (valid_long_expandos && mb_strlen_range(start, end) > 1)
  {
    for (size_t i = 0; valid_long_expandos[i].name != NULL; ++i)
    {
      const int len = strlen(valid_long_expandos[i].name);
      if (strncmp(start, valid_long_expandos[i].name, len) == 0)
      {
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

static struct ExpandoNode *
parse_node(const char *s, enum ExpandoConditionStart condition_start,
           const char **parsed_until, const struct ExpandoFormatCallback *valid_short_expandos,
           const struct ExpandoFormatCallback *valid_two_char_expandos,
           const struct ExpandoFormatCallback *valid_long_expandos,
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
        // TODO(g0mb4): handle {name} expandos!
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
          ++s;
          dt = DT_SENDER_SEND_TIME;
          end = skip_until(s, "}");
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
          dt = DT_LOCAL_SEND_TIME;
          end = skip_until(s, "]");
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
          ++s;
          dt = DT_LOCAL_RECIEVE_TIME;
          end = skip_until(s, ")");
          if (*end != ')')
          {
            error->position = end;
            snprintf(error->message, sizeof(error->message), "Missing ')'.");
            return NULL;
          }
        }

        *parsed_until = end + 1;
        return new_date_node(start, end, dt, ignore_locale);
      }
      // padding
      else if (*s == '|' || *s == '>' || *s == '*')
      {
        enum ExpandoPadType pt = 0;
        if (*s == '|')
        {
          pt = PT_FILL;
        }
        else if (*s == '>')
        {
          pt = PT_HARD_FILL;
        }
        // '*'
        else
        {
          pt = PT_SOFT_FILL;
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

        const char *next = NULL;
        struct ExpandoNode *condition = parse_node(s, CON_START, &next, valid_short_expandos,
                                                   valid_two_char_expandos,
                                                   valid_long_expandos, error);
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

        struct ExpandoNode *if_true = parse_node(s, CON_NO_CONDITION, &next,
                                                 valid_short_expandos, valid_two_char_expandos,
                                                 valid_long_expandos, error);
        if (if_true == NULL)
        {
          return NULL;
        }
        if ((old_style && *next == '?') || (!old_style && *next == '>'))
        {
          *parsed_until = next + 1;
          return new_condition_node(condition, if_true, NULL);
        }
        else if (*next == '&')
        {
          s = next + 1;
          struct ExpandoNode *if_false = parse_node(s, CON_NO_CONDITION, &next,
                                                    valid_short_expandos, valid_two_char_expandos,
                                                    valid_long_expandos, error);
          if (if_true == NULL)
          {
            return NULL;
          }

          if (old_style)
          {
            if (*next != '?')
            {
              error->position = next;
              snprintf(error->message, sizeof(error->message), "Missing '?'.");
              return NULL;
            }
          }
          else
          {
            if (*next != '>')
            {
              error->position = next;
              snprintf(error->message, sizeof(error->message), "Missing '>'.");
              return NULL;
            }
          }

          *parsed_until = next + 1;
          return new_condition_node(condition, if_true, if_false);
        }
        else
        {
          error->position = next;
          snprintf(error->message, sizeof(error->message), "Invalid conditional.");
          return NULL;
        }
      }
      // index format hook
      else if (*s == '@')
      {
        ++s;
        const char *end = skip_until(s, "@");

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
        const char *expando_end = skip_classic_expando(format_end, valid_two_char_expandos);
        const struct ExpandoFormat *format = parse_format(s, format_end, error);
        if (error->position != NULL)
        {
          return NULL;
        }

        expando_format_callback callback = check_if_expando_is_valid(
            format_end, expando_end, valid_short_expandos,
            valid_two_char_expandos, valid_long_expandos, error);
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
      const char *end = skip_until(s, "%");
      *parsed_until = end;
      return new_text_node(s, end);
    }
  }

  error->position = s;
  snprintf(error->message, sizeof(error->message), "Internal parsing error.");
  return NULL;
}

static void print_empty_node(FILE *fp, const struct ExpandoNode *n, int indent)
{
  fprintf(fp, "%*sEMPTY\n", indent, "");
}

static void print_text_node(FILE *fp, const struct ExpandoTextNode *n, int indent)
{
  const int len = n->end - n->start;
  fprintf(fp, "%*sTEXT: `%.*s`\n", indent, "", len, n->start);
}

static void print_expando_node(FILE *fp, const struct ExpandoExpandoNode *n, int indent)
{
  if (n->format)
  {
    const int elen = n->end - n->start;
    const struct ExpandoFormat *f = n->format;
    fprintf(fp, "%*sEXPANDO: `%.*s`", indent, "", elen, n->start);

    const char *just = f->justification == FMT_J_RIGHT ? "RIGHT" : "LEFT";
    fprintf(fp, " (min=%d, max=%d, just=%s, leader=`%c`)\n", f->min, f->max, just, f->leader);
  }
  else
  {
    const int len = n->end - n->start;
    fprintf(fp, "%*sEXPANDO: `%.*s`\n", indent, "", len, n->start);
  }
}

static void print_date_node(FILE *fp, const struct ExpandoDateNode *n, int indent)
{
  const int len = n->end - n->start;
  const char *dt = NULL;

  switch (n->date_type)
  {
    case DT_SENDER_SEND_TIME:
      dt = "SENDER_SEND_TIME";
      break;

    case DT_LOCAL_SEND_TIME:
      dt = "DT_LOCAL_SEND_TIME";
      break;

    case DT_LOCAL_RECIEVE_TIME:
      dt = "DT_LOCAL_RECIEVE_TIME";
      break;

    default:
      assert(0 && "Unknown date type.");
  }

  fprintf(fp, "%*sDATE: `%.*s` (type=%s, ignore_locale=%d)\n", indent, "", len,
          n->start, dt, n->ingnore_locale);
}

static void print_pad_node(FILE *fp, const struct ExpandoPadNode *n, int indent)
{
  const char *pt = NULL;
  switch (n->pad_type)
  {
    case PT_FILL:
      pt = "FILL";
      break;

    case PT_HARD_FILL:
      pt = "HARD_FILL";
      break;

    case PT_SOFT_FILL:
      pt = "SOFT_FILL";
      break;

    default:
      assert(0 && "Unknown pad type.");
  }

  const int len = n->end - n->start;
  fprintf(fp, "%*sPAD: `%.*s` (type=%s)\n", indent, "", len, n->start, pt);
}

static void print_condition_node(FILE *fp, const struct ExpandoConditionNode *n, int indent)
{
  fprintf(fp, "%*sCONDITION: ", indent, "");
  print_node(fp, n->condition, indent);
  fprintf(fp, "%*sIF TRUE : ", indent + 4, "");
  print_node(fp, n->if_true, indent + 4);

  if (n->if_false)
  {
    fprintf(fp, "%*sIF FALSE: ", indent + 4, "");
    print_node(fp, n->if_false, indent + 4);
  }
}

static void print_index_format_hook_node(FILE *fp,
                                         const struct ExpandoIndexFormatHookNode *n, int indent)
{
  const int len = n->end - n->start;
  fprintf(fp, "%*sINDEX FORMAT HOOK: `%.*s`\n", indent, "", len, n->start);
}

static void print_node(FILE *fp, const struct ExpandoNode *n, int indent)
{
  if (n == NULL)
  {
    fprintf(fp, "<null>\n");
    return;
  }

  switch (n->type)
  {
    case NT_EMPTY:
    {
      print_empty_node(fp, n, indent);
    }
    break;

    case NT_TEXT:
    {
      const struct ExpandoTextNode *nn = (const struct ExpandoTextNode *) n;
      print_text_node(fp, nn, indent);
    }
    break;

    case NT_EXPANDO:
    {
      const struct ExpandoExpandoNode *nn = (const struct ExpandoExpandoNode *) n;
      print_expando_node(fp, nn, indent);
    }
    break;

    case NT_DATE:
    {
      const struct ExpandoDateNode *nn = (const struct ExpandoDateNode *) n;
      print_date_node(fp, nn, indent);
    }
    break;

    case NT_PAD:
    {
      const struct ExpandoPadNode *nn = (const struct ExpandoPadNode *) n;
      print_pad_node(fp, nn, indent);
    }
    break;

    case NT_INDEX_FORMAT_HOOK:
    {
      const struct ExpandoIndexFormatHookNode *nn = (const struct ExpandoIndexFormatHookNode *) n;
      print_index_format_hook_node(fp, nn, indent);
    }
    break;

    case NT_CONDITION:
    {
      const struct ExpandoConditionNode *nn = (const struct ExpandoConditionNode *) n;
      print_condition_node(fp, nn, indent);
    }
    break;

    default:
      assert(0 && "Unknown node.");
  }
}

static void free_node(struct ExpandoNode *n)
{
  if (n == NULL)
  {
    return;
  }

  switch (n->type)
  {
    case NT_EMPTY:
    {
      free_empty_node(n);
    }
    break;

    case NT_TEXT:
    {
      struct ExpandoTextNode *nn = (struct ExpandoTextNode *) n;
      free_text_node(nn);
    }
    break;

    case NT_EXPANDO:
    {
      struct ExpandoExpandoNode *nn = (struct ExpandoExpandoNode *) n;
      free_expando_node(nn);
    }
    break;

    case NT_DATE:
    {
      struct ExpandoDateNode *nn = (struct ExpandoDateNode *) n;
      free_date_node(nn);
    }
    break;

    case NT_PAD:
    {
      struct ExpandoPadNode *nn = (struct ExpandoPadNode *) n;
      free_pad_node(nn);
    }
    break;

    case NT_INDEX_FORMAT_HOOK:
    {
      struct ExpandoIndexFormatHookNode *nn = (struct ExpandoIndexFormatHookNode *) n;
      free_index_format_hook_node(nn);
    }
    break;

    case NT_CONDITION:
    {
      struct ExpandoConditionNode *nn = (struct ExpandoConditionNode *) n;
      free_condition_node(nn);
    }
    break;

    default:
      assert(0 && "Unknown node.");
  }
}

void expando_tree_parse(struct ExpandoNode **root, const char **string,
                        const struct ExpandoFormatCallback *valid_short_expandos,
                        const struct ExpandoFormatCallback *valid_two_char_expandos,
                        const struct ExpandoFormatCallback *valid_long_expandos,
                        struct ExpandoParseError *error)
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
    struct ExpandoNode *n = parse_node(start, CON_NO_CONDITION, &end,
                                       valid_short_expandos, valid_two_char_expandos,
                                       valid_long_expandos, error);

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
  while (n)
  {
    struct ExpandoNode *f = n;
    n = n->next;
    free_node(f);
  }
}

void expando_tree_print(FILE *fp, struct ExpandoNode **root)
{
  const struct ExpandoNode *n = *root;
  while (n)
  {
    print_node(fp, n, 0);
    n = n->next;
  }
}
