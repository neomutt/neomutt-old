#include "common.h"

void test_expando_simple_expando(void)
{
  const char *text = "%a %b";
  const char *input = text;
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, input, NULL, NULL, NULL, &error);

  TEST_CHECK(error.position == NULL);
  check_expando_node(get_nth_node(&root, 0), "a", NULL);
  check_text_node(get_nth_node(&root, 1), " ");
  check_expando_node(get_nth_node(&root, 2), "b", NULL);

  expando_tree_free(&root);
}