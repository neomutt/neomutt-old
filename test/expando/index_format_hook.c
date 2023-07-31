#include "common.h"

void test_expando_index_format_hook(void)
{
  const char *input = "%@hook@";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, &input, NULL, NULL, NULL, &error);

  TEST_CHECK(error.position == NULL);
  check_index_hook_node(get_nth_node(&root, 0), "hook");

  expando_tree_free(&root);
}