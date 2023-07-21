/**
 * DISCLAIMER: This is a work-in-progress code.
 * 
 * Tóth János
 */

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

static const char *valid_2char_expandos[] = { "aa", "ab", NULL };

enum NodeType
{
  NT_TEXT,
  NT_EXPANDO,
  NT_DATE,
  NT_PAD,
  NT_CONDITION,
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

static struct Node *new_text_node(const char *start, const char *end)
{
  struct TextNode *node = malloc(sizeof(struct TextNode));

  VERIFY(node != NULL);

  memset(node, 0, sizeof(struct TextNode));

  node->type = NT_TEXT;
  node->start = start;
  node->end = end;

  return (struct Node *) node;
}

static struct Node *new_expando_node(const char *start, const char *end,
                                     const struct Format *format)
{
  struct ExpandoNode *node = malloc(sizeof(struct ExpandoNode));

  VERIFY(node != NULL);

  memset(node, 0, sizeof(struct ExpandoNode));

  node->type = NT_EXPANDO;
  node->start = start;
  node->end = end;
  node->format = format;

  return (struct Node *) node;
}

static struct Node *new_date_node(const char *start, const char *end,
                                  enum DateType date_type, bool ingnore_locale)
{
  struct DateNode *node = malloc(sizeof(struct DateNode));

  VERIFY(node != NULL);

  memset(node, 0, sizeof(struct DateNode));

  node->type = NT_DATE;
  node->start = start;
  node->end = end;
  node->date_type = date_type;
  node->ingnore_locale = ingnore_locale;

  return (struct Node *) node;
}

static struct Node *new_pad_node(enum PadType pad_type, char pad_char)
{
  struct PadNode *node = malloc(sizeof(struct PadNode));

  VERIFY(node != NULL);

  memset(node, 0, sizeof(struct PadNode));

  node->type = NT_PAD;
  node->pad_type = pad_type;
  node->pad_char = pad_char;

  return (struct Node *) node;
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

static inline bool is_valid_classic_expando(char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static const char *skip_until_classic_expando(const char *start)
{
  while (*start && !is_valid_classic_expando(*start))
  {
    ++start;
  }

  return start;
}

static const char *skip_classic_expando(const char *s)
{
  VERIFY(is_valid_classic_expando(*s));

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

static const struct Format *parse_format(const char *start, const char *end)
{
  if (start == end)
  {
    return NULL;
  }

  struct Format *format = malloc(sizeof(struct Format));

  VERIFY(format != NULL);

  memset(format, 0, sizeof(struct Format));
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
        is_min = false;
        ++start;
        break;

      // number
      default:
        char *end_ptr;
        int number = strtol(start, &end_ptr, 10);

        // NOTE: start is NOT null-terminated
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

  return format;
}

static void parse(struct Node **root, const char *s)
{
  while (*s)
  {
    // expando
    if (*s == '%')
    {
      s++;
      // conditional
      if (*s == '<')
      {
        TODO();
        s = skip_until(s, ">");
      }
      // dates
      else if (*s == '{' || *s == '[' || *s == '(')
      {
        // TODO: handle {name} expandos!
        bool ignore_locale = *(s + 1) == '!';
        enum DateType dt = DT_SENDER_SEND_TIME;
        const char *end = NULL;

        if (*s == '{')
        {
          ++s;
          dt = DT_SENDER_SEND_TIME;
          end = skip_until(s, "}");
          VERIFY(*end == '}');
        }
        else if (*s == '[')
        {
          ++s;
          dt = DT_LOCAL_SEND_TIME;
          end = skip_until(s, "]");
          VERIFY(*end == ']');
        }
        // '('
        else
        {
          ++s;
          dt = DT_LOCAL_RECIEVE_TIME;
          end = skip_until(s, ")");
          VERIFY(*end == ')');
        }

        append_node(root, new_date_node(s, end, dt, ignore_locale));
        s = end + 1;
      }
      // padding
      else if (*s == '|' || *s == '>' || *s == '*')
      {
        enum PadType pt = PT_FILL;
        if (*s == '|')
        {
          pt = PT_FILL;
        }
        else if (*s == '>')
        {
          pt = PT_HARD_FILL;
        }
        else
        {
          pt = PT_SOFT_FILL;
        }

        char pad_char = *(s + 1);
        VERIFY(pad_char != '\0');

        append_node(root, new_pad_node(pt, pad_char));
        s += 2;
      }
      // "%"
      else if (*s == '%')
      {
        append_node(root, new_text_node(s, s + 1));
        s++;
      }
      // classic expandos: %X
      else
      {
        const char *format_end = skip_until_classic_expando(s);
        const char *expando_end = skip_classic_expando(format_end);
        const struct Format *format = parse_format(s, format_end);
        append_node(root, new_expando_node(format_end, expando_end, format));
        s = expando_end;
      }
    }
    // text
    else
    {
      const char *end = skip_until(s, "%");
      append_node(root, new_text_node(s, end));
      s = end;
    }
  }
}

static void print(struct Node **root)
{
  struct Node *n = *root;
  while (n)
  {
    switch (n->type)
    {
      case NT_TEXT:
      {
        const struct TextNode *t = (const struct TextNode *) n;
        int len = t->end - t->start;
        printf("TEXT: `%.*s`\n", len, t->start);
      }
      break;

      case NT_EXPANDO:
      {
        const struct ExpandoNode *e = (const struct ExpandoNode *) n;
        if (e->format)
        {
          int elen = e->end - e->start;
          const struct Format *f = e->format;
          int flen = f->end - f->start;
          printf("EXPANDO: `%.*s`", elen, e->start);

          const char *just = f->justification == FMT_J_RIGHT ? "RIGHT" : "LEFT";
          printf(" (`%.*s`: min=%d, max=%d, just=%s, leader=`%c`)\n", flen,
                 f->start, f->min, f->max, just, f->leader);
        }
        else
        {
          int elen = e->end - e->start;
          printf("EXPANDO: `%.*s`\n", elen, e->start);
        }
      }
      break;

      case NT_DATE:
      {
        const struct DateNode *d = (const struct DateNode *) n;
        int len = d->end - d->start;

        const char *dt = NULL;
        switch (d->date_type)
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
            ERROR("Unknown date type: %d\n", d->date_type);
        }

        printf("DATE: `%.*s` (type=%s, ignore_locale=%d)\n", len, d->start, dt,
               d->ingnore_locale);
      }
      break;

      case NT_PAD:
      {
        const struct PadNode *p = (const struct PadNode *) n;

        const char *pt = NULL;
        switch (p->pad_type)
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
            ERROR("Unknown pad type: %d\n", p->pad_type);
        }

        printf("PAD: `%c` (type=%s)\n", p->pad_char, pt);
      }
      break;

      case NT_CONDITION:
        TODO();
        break;

      default:
        ERROR("Unknown node: %d\n", n->type);
    };
    n = n->next;
  }
}

static void free_node(struct Node *n)
{
  switch (n->type)
  {
    case NT_TEXT:
    case NT_DATE:
    case NT_PAD:
      free(n);
      break;

    case NT_EXPANDO:
    {
      struct ExpandoNode *e = (struct ExpandoNode *) n;
      if (e->format)
      {
        free((struct Format *) e->format);
      }
      free(e);
    }
    break;

    case NT_CONDITION:
      TODO();
      break;

    default:
      ERROR("Unknown node: %d\n", n->type);
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

int main(void)
{
  //const char *text = "test text";
  //const char *text = "test text%% %a %4b";
  //const char *text = "%X %8X %-8X %08X %.8X %8.8X %-8.8X";
  //const char *text = "test text%% %aa %4ab %bb";
  //const char *text = " %[%b %d]  %{!%b %d} %(%b %d)";
  const char *text = "%|A %>B %*C";
  // const char* text = "%4C %[%b %d %H:%M] %-15.15L (%<l?%4l>) %s";

  printf("%s\n", text);

  static struct Node *root = NULL;

  parse(&root, text);

  print(&root);

  free_tree(&root);

  return 0;
}