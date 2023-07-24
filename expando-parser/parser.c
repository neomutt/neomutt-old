/**
 * DISCLAIMER: This is a work-in-progress code.
 * 
 * Tóth János
 */

#include "parser.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TODO()                                                                 \
  fprintf(stderr, "%s: %d: TODO\n", __FILE__, __LINE__);                       \
  exit(1)

#define ERROR(...)                                                             \
  fprintf(stderr, "ERROR: " __VA_ARGS__);                                      \
  exit(1)

#define VERIFY(b)                                                                 \
  if (!(b))                                                                       \
  {                                                                               \
    fprintf(stderr, "%s: %d: VERIFICATION FAILED: %s\n", __FILE__, __LINE__, #b); \
    exit(1);                                                                      \
  }

static void free_node(struct Node *n);
static void print_node(FILE *fp, const struct Node *n, int indent);

static struct Node *new_text_node(const char *start, const char *end)
{
  struct TextNode *node = calloc(1, sizeof(struct TextNode));

  VERIFY(node != NULL);

  node->type = NT_TEXT;
  node->start = start;
  node->end = end;

  return (struct Node *) node;
}

static struct Node *new_expando_node(const char *start, const char *end,
                                     const struct Format *format)
{
  struct ExpandoNode *node = calloc(1, sizeof(struct ExpandoNode));

  VERIFY(node != NULL);

  node->type = NT_EXPANDO;
  node->start = start;
  node->end = end;
  node->format = format;

  return (struct Node *) node;
}

static struct Node *new_date_node(const char *start, const char *end,
                                  enum DateType date_type, bool ingnore_locale)
{
  struct DateNode *node = calloc(1, sizeof(struct DateNode));

  VERIFY(node != NULL);

  node->type = NT_DATE;
  node->start = start;
  node->end = end;
  node->date_type = date_type;
  node->ingnore_locale = ingnore_locale;

  return (struct Node *) node;
}

static struct Node *new_pad_node(enum PadType pad_type, char pad_char)
{
  struct PadNode *node = calloc(1, sizeof(struct PadNode));

  VERIFY(node != NULL);

  node->type = NT_PAD;
  node->pad_type = pad_type;
  node->pad_char = pad_char;

  return (struct Node *) node;
}

static struct Node *new_condition_node(struct Node *condition,
                                       struct Node *if_true, struct Node *if_false)
{
  VERIFY(condition != NULL);
  VERIFY(if_true != NULL);

  struct ConditionNode *node = calloc(1, sizeof(struct ConditionNode));

  VERIFY(node != NULL);

  node->type = NT_CONDITION;
  node->condition = condition;
  node->if_true = if_true;
  node->if_false = if_false;

  return (struct Node *) node;
}

static struct Node *new_index_format_hook_node(const char *start, const char *end)
{
  struct IndexFormatHookNode *node = calloc(1, sizeof(struct IndexFormatHookNode));

  VERIFY(node != NULL);

  node->type = NT_INDEX_FORMAT_HOOK;
  node->start = start;
  node->end = end;

  return (struct Node *) node;
}

static void free_text_node(struct TextNode *n)
{
  free(n);
}

static void free_date_node(struct DateNode *n)
{
  free(n);
}

static void free_expando_node(struct ExpandoNode *n)
{
  if (n->format)
  {
    free((struct Format *) n->format);
  }

  free(n);
}

static void free_pad_node(struct PadNode *n)
{
  free(n);
}

static void free_condition_node(struct ConditionNode *n)
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

  free(n);
}

static void free_index_format_hook_node(struct IndexFormatHookNode *n)
{
  free(n);
}

static void append_node(struct Node **root, struct Node *new_node)
{
  if (!*root)
  {
    *root = new_node;
    return;
  }

  struct Node *n = *root;
  while (n->next != NULL)
  {
    n = n->next;
  }

  n->next = new_node;
}

static const char *skip_until_classic_expando(const char *start)
{
  while (*start && !isalpha(*start))
  {
    ++start;
  }

  return start;
}

static const char *skip_classic_expando(const char *s)
{
  static const char *valid_2char_expandos[] = { "aa", "ab", NULL };

  for (size_t i = 0; valid_2char_expandos[i] != NULL; ++i)
  {
    if (valid_2char_expandos[i][0] == *s && valid_2char_expandos[i][1] == *(s + 1))
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

static inline bool is_valid_number(char c)
{
  return (c >= '0' && c <= '9');
}

static const struct Format *parse_format(const char *start, const char *end,
                                         struct ParseError *error)
{
  if (start == end)
  {
    return NULL;
  }

  struct Format *format = calloc(1, sizeof(struct Format));

  VERIFY(format != NULL);

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

      case '0':
        format->leader = '0';
        ++start;
        break;

      case '.':
      {
        if (!isdigit(*(start + 1)))
        {
          error->position = start;
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

        VERIFY(end_ptr <= end);

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

static struct Node *parse_node(const char *s, enum ConditionStart condition_start,
                               const char **parsed_until, struct ParseError *error)
{
  while (*s)
  {
    if (*s == '%' || (condition_start == CON_START && (*s == '?' || *s == '<')))
    {
      s++;
      // dates
      if (*s == '{' || *s == '[' || *s == '(')
      {
        // TODO: handle {name} expandos!
        bool ignore_locale = *(s + 1) == '!';

        enum DateType dt = 0;
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
        return new_date_node(s, end, dt, ignore_locale);
      }
      // padding
      else if (*s == '|' || *s == '>' || *s == '*')
      {
        enum PadType pt = 0;
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

        char pad_char = *(s + 1);

        // NOTE: all characters are valid?
        if (pad_char == '\0')
        {
          error->position = *(s + 1);
          snprintf(error->message, sizeof(error->message), "Invalid padding character.");
          return NULL;
        }

        *parsed_until = s + 2;
        return new_pad_node(pt, pad_char);
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
        struct Node *condition = parse_node(s, CON_START, &next, error);
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

        struct Node *if_true = parse_node(s, CON_NO_CONDITION, &next, error);
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
          struct Node *if_false = parse_node(s, CON_NO_CONDITION, &next, error);
          if (if_true == NULL)
          {
            return NULL;
          }

          if (old_style)
          {
            if (*next != '?')
            {
              error->position = *next;
              snprintf(error->message, sizeof(error->message), "Missing '?'.");
              return NULL;
            }
          }
          else
          {
            if (*next != '>')
            {
              error->position = *next;
              snprintf(error->message, sizeof(error->message), "Missing '>'.");
              return NULL;
            }
          }

          *parsed_until = next + 1;
          return new_condition_node(condition, if_true, if_false);
        }
        else
        {
          error->position = *next;
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
          error->position = *end;
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
        const char *expando_end = skip_classic_expando(format_end);
        const struct Format *format = parse_format(s, format_end, error);
        if (error->position != NULL)
        {
          return NULL;
        }

        *parsed_until = expando_end;
        return new_expando_node(format_end, expando_end, format);
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

  error->position = *s;
  snprintf(error->message, sizeof(error->message), "Internal parsing error.");
  return NULL;
}

static void print_text_node(FILE *fp, const struct TextNode *n, int indent)
{
  int len = n->end - n->start;
  fprintf(fp, "%*sTEXT: `%.*s`\n", indent, "", len, n->start);
}

static void print_expando_node(FILE *fp, const struct ExpandoNode *n, int indent)
{
  if (n->format)
  {
    int elen = n->end - n->start;
    const struct Format *f = n->format;
    int flen = n->end - n->start;
    fprintf(fp, "%*sEXPANDO: `%.*s`", indent, "", elen, n->start);

    const char *just = f->justification == FMT_J_RIGHT ? "RIGHT" : "LEFT";
    fprintf(fp, " (min=%d, max=%d, just=%s, leader=`%c`)\n", f->min, f->max, just, f->leader);
  }
  else
  {
    int len = n->end - n->start;
    fprintf(fp, "%*sEXPANDO: `%.*s`\n", indent, "", len, n->start);
  }
}

static void print_date_node(FILE *fp, const struct DateNode *n, int indent)
{
  int len = n->end - n->start;
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
      ERROR("Unknown date type: %d\n", n->date_type);
  }

  fprintf(fp, "%*sDATE: `%.*s` (type=%s, ignore_locale=%d)\n", indent, "", len,
          n->start, dt, n->ingnore_locale);
}

static void print_pad_node(FILE *fp, const struct PadNode *n, int indent)
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
      ERROR("Unknown pad type: %d\n", n->pad_type);
  }

  fprintf(fp, "%*sPAD: `%c` (type=%s)\n", indent, "", n->pad_char, pt);
}

static void print_condition_node(FILE *fp, const struct ConditionNode *n, int indent)
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

static void print_index_format_hook_node(FILE *fp, const struct IndexFormatHookNode *n, int indent)
{
  int len = n->end - n->start;
  fprintf(fp, "%*sINDEX FORMAT HOOK: `%.*s`\n", indent, "", len, n->start);
}

static void print_node(FILE *fp, const struct Node *n, int indent)
{
  if (n == NULL)
  {
    fprintf(fp, "<null>\n");
    return;
  }

  switch (n->type)
  {
    case NT_TEXT:
    {
      const struct TextNode *nn = (const struct TextNode *) n;
      print_text_node(fp, nn, indent);
    }
    break;

    case NT_EXPANDO:
    {
      const struct ExpandoNode *nn = (const struct ExpandoNode *) n;
      print_expando_node(fp, nn, indent);
    }
    break;

    case NT_DATE:
    {
      const struct DateNode *nn = (const struct DateNode *) n;
      print_date_node(fp, nn, indent);
    }
    break;

    case NT_PAD:
    {
      const struct PadNode *nn = (const struct PadNode *) n;
      print_pad_node(fp, nn, indent);
    }
    break;

    case NT_INDEX_FORMAT_HOOK:
    {
      const struct IndexFormatHookNode *nn = (const struct IndexFormatHookNode *) n;
      print_index_format_hook_node(fp, nn, indent);
    }
    break;

    case NT_CONDITION:
    {
      const struct ConditionNode *nn = (const struct ConditionNode *) n;
      print_condition_node(fp, nn, indent);
    }
    break;

    default:
      ERROR("Unknown node: %d\n", n->type);
  }
}

static void free_node(struct Node *n)
{
  if (n == NULL)
  {
    return;
  }

  switch (n->type)
  {
    case NT_TEXT:
    {
      struct TextNode *nn = (struct TextNode *) n;
      free_text_node(nn);
    }
    break;

    case NT_EXPANDO:
    {
      struct ExpandoNode *nn = (struct ExpandoNode *) n;
      free_expando_node(nn);
    }
    break;

    case NT_DATE:
    {
      struct DateNode *nn = (struct DateNode *) n;
      free_date_node(nn);
    }
    break;

    case NT_PAD:
    {
      struct PadNode *nn = (struct PadNode *) n;
      free_pad_node(nn);
    }
    break;

    case NT_INDEX_FORMAT_HOOK:
    {
      struct IndexFormatHookNode *nn = (struct IndexFormatHookNode *) n;
      free_index_format_hook_node(nn);
    }
    break;

    case NT_CONDITION:
    {
      struct ConditionNode *nn = (struct ConditionNode *) n;
      free_condition_node(nn);
    }
    break;

    default:
      ERROR("Unknown node: %d\n", n->type);
  }
}

void parse_tree(struct Node **root, const char *s, struct ParseError *error)
{
  const char *end = NULL;
  while (*s)
  {
    struct Node *n = parse_node(s, CON_NO_CONDITION, &end, error);

    if (n == NULL)
    {
      break;
    }

    append_node(root, n);
    s = end;
  }
}

void free_tree(struct Node **root)
{
  struct Node *n = *root;
  while (n)
  {
    struct Node *f = n;
    n = n->next;
    free_node(f);
  }
}

void print_tree(FILE *fp, struct Node **root)
{
  const struct Node *n = *root;
  while (n)
  {
    print_node(fp, n, 0);
    n = n->next;
  }
}
