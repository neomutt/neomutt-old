#include "common.h"

void test_expando_empty(void)
{
  const char *input = "";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, &input, NULL, NULL, NULL, &error);

  TEST_CHECK(error.position == NULL);
  check_empty_node(get_nth_node(&root, 0));

  expando_tree_free(&root);
}