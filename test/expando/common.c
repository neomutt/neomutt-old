#include "common.h"

struct ExpandoNode *get_nth_node(struct ExpandoNode **root, int n)
{
  TEST_CHECK(*root != NULL);

  struct ExpandoNode *node = *root;
  int i = 0;

  while (node)
  {
    if (i++ == n)
    {
      return node;
    }

    node = node->next;
  }

  if (!TEST_CHECK(0))
  {
    TEST_MSG("Node is not found\n");
  }

  return NULL;
}

void check_text_node(struct ExpandoNode *node, const char *text)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == NT_TEXT);

  struct ExpandoTextNode *t = (struct ExpandoTextNode *) node;

  const int n = strlen(text);
  const int m = (int) (t->end - t->start);
  TEST_CHECK(n == m);
  TEST_CHECK(strncmp(t->start, text, n) == 0);
}

void check_expando_node(struct ExpandoNode *node, const char *expando,
                        const struct ExpandoFormat *format)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == NT_EXPANDO);

  struct ExpandoExpandoNode *e = (struct ExpandoExpandoNode *) node;
  const int n = strlen(expando);
  const int m = (int) (e->end - e->start);

  TEST_CHECK(n == m);
  TEST_CHECK(strncmp(e->start, expando, n) == 0);

  if (format == NULL)
  {
    TEST_CHECK(e->format == NULL);
  }
  else
  {
    TEST_CHECK(e->format != NULL);
    TEST_CHECK(e->format->justification == format->justification);
    TEST_CHECK(e->format->leader == format->leader);
    TEST_CHECK(e->format->min == format->min);
    TEST_CHECK(e->format->max == format->max);
  }
}

void check_pad_node(struct ExpandoNode *node, char pad_char, enum ExpandoPadType pad_type)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == NT_PAD);

  struct ExpandoPadNode *p = (struct ExpandoPadNode *) node;

  TEST_CHECK(p->pad_char == pad_char);
  TEST_CHECK(p->pad_type == pad_type);
}

void check_date_node(struct ExpandoNode *node, const char *inner_text,
                     enum ExpandoDateType date_type, bool ingnore_locale)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == NT_DATE);

  struct ExpandoDateNode *d = (struct ExpandoDateNode *) node;

  const int n = strlen(inner_text);
  const int m = (int) (d->end - d->start);
  TEST_CHECK(n == m);
  TEST_CHECK(strncmp(d->start, inner_text, n) == 0);

  TEST_CHECK(d->date_type == date_type);
  TEST_CHECK(d->ingnore_locale == ingnore_locale);
}

void check_condition_node_head(struct ExpandoNode *node)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == NT_CONDITION);
}

void check_index_hook_node(struct ExpandoNode *node, const char *name)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == NT_INDEX_FORMAT_HOOK);

  struct ExpandoIndexFormatHookNode *i = (struct ExpandoIndexFormatHookNode *) node;

  const int n = strlen(name);
  const int m = (int) (i->end - i->start);
  TEST_CHECK(n == m);
  TEST_CHECK(strncmp(i->start, name, n) == 0);
}