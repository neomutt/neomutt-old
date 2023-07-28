#include "common.h"

void test_expando_padding(void)
{
  const char *text = "%|A %>B %*C";
  const char *input = text;
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, input, NULL, NULL, NULL, &error);

  TEST_CHECK(error.position == NULL);
  check_pad_node(get_nth_node(&root, 0), 'A', PT_FILL);
  check_text_node(get_nth_node(&root, 1), " ");
  check_pad_node(get_nth_node(&root, 2), 'B', PT_HARD_FILL);
  check_text_node(get_nth_node(&root, 3), " ");
  check_pad_node(get_nth_node(&root, 4), 'C', PT_SOFT_FILL);

  expando_tree_free(&root);
}