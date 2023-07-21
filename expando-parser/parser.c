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
  if (!b)                                                                         \
  {                                                                               \
    fprintf(stderr, "%s: %d: VERIFICATION FAILED: %s\n", __FILE__, __LINE__, #b); \
    exit(1);                                                                      \
  }

enum NodeType
{
  NT_TEXT,
  NT_EXPANDO,
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

struct ExpandoNode
{
  enum NodeType type;
  struct Node *next;

  // can be used either fo %n or {name}
  const char *expando_start;
  const char *expando_end;
  // TODO: proper struct for these
  const char *fmt_start;
  const char *fmt_end;
};

static struct Node *new_text_node(const char *start, const char *end)
{
  struct TextNode *node = malloc(sizeof(struct TextNode));
  memset(node, 0, sizeof(struct TextNode));

  node->type = NT_TEXT;
  node->start = start;
  node->end = end;

  return (struct Node *) node;
}

static struct Node *new_expando_node(const char *expando_start, const char *expando_end,
                                     const char *fmt_start, const char *fmt_end)
{
  struct ExpandoNode *node = malloc(sizeof(struct ExpandoNode));
  memset(node, 0, sizeof(struct ExpandoNode));

  node->type = NT_EXPANDO;
  node->expando_start = expando_start;
  node->expando_end = expando_end;
  // TODO: proper struct for these
  node->fmt_start = fmt_start;
  node->fmt_end = fmt_end;

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
  s++;

  // TODO: handle 2 char expandos here
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

static void parse(struct Node **root, const char *s)
{
  while (*s)
  {
    if (*s == '%')
    {
      s++;
      if (*s == '<')
      {
        TODO();
        s = skip_until(s, ">");
      }
      else
      {
        // classic expandos: %X
        const char *format_end = skip_until_classic_expando(s);
        const char *expando_end = skip_classic_expando(format_end);

        append_node(root, new_expando_node(format_end, expando_end, s, format_end));
        s = expando_end;
      }
    }
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
        const struct TextNode *t = (const struct TextNode *) n;
        int len = t->end - t->start;
        printf("TEXT: `%.*s`\n", len, t->start);
        break;
      case NT_EXPANDO:
        const struct ExpandoNode *e = (const struct ExpandoNode *) n;
        if (e->fmt_end > e->fmt_start)
        {
          int elen = e->expando_end - e->expando_start;
          int flen = e->fmt_end - e->fmt_start;
          printf("EXPANDO:`%.*s`, `%.*s`\n", elen, e->expando_start, flen, e->fmt_start);
        }
        else
        {
          int elen = e->expando_end - e->expando_start;
          printf("EXPANDO: `%.*s`\n", elen, e->expando_start);
        }

        break;
      case NT_CONDITION:
        TODO();
        break;
    };
    n = n->next;
  }
}

void free_tree(struct Node **root)
{
  struct Node *n = *root;
  while (n)
  {
    struct Node *f = n;
    n = n->next;

    // TODO: node specific free
    free(f);
  }
};

int main(void)
{
  //const char *text = "test text";
  const char *text = "test text %a %b";
  // const char* text = "%4C %[%b %d %H:%M] %-15.15L (%<l?%4l>) %s";

  printf("%s\n", text);

  static struct Node *root = NULL;

  parse(&root, text);

  print(&root);

  free_tree(&root);

  return 0;
}