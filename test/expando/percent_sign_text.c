#include "common.h"

void test_expando_percent_sign_text(void)
{
  const char *input = "percent %%";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, &input, NULL, NULL, NULL, &error);

  TEST_CHECK(error.position == NULL);
  check_text_node(get_nth_node(&root, 0), "percent ");
  check_text_node(get_nth_node(&root, 1), "%");

  expando_tree_free(&root);
}