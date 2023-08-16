#include "common.h"

void test_expando_padding(void)
{
  const char *input = "%|A %>B %*C";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, &input, EFMTI_FORMAT_COUNT_OR_DEBUG, &error);

  TEST_CHECK(error.position == NULL);
  check_pad_node(get_nth_node(&root, 0), "A", EPT_FILL_EOL);
  check_text_node(get_nth_node(&root, 1), " ");
  check_pad_node(get_nth_node(&root, 2), "B", EPT_HARD_FILL);
  check_text_node(get_nth_node(&root, 3), " ");
  check_pad_node(get_nth_node(&root, 4), "C", EPT_SOFT_FILL);

  expando_tree_free(&root);
}