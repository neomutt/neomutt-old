#include "common.h"

struct ExpandoNode *get_nth_node(struct ExpandoNode **root, int n)
{
  TEST_CHECK(*root != NULL);

  struct ExpandoNode *node = *root;
  int i = 0;

  while (node)
  {
    TEST_CHECK(node != NULL);

    if (i == n)
    {
      return node;
    }

    node = node->next;
    ++i;
  }

  TEST_CHECK(0);
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