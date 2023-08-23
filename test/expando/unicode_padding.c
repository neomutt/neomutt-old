#include "common.h"

void test_expando_unicode_padding(void)
{
  const char *input = "%|😀";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, &input, EFMTI_FORMAT_COUNT_OR_DEBUG, &error);

  TEST_CHECK(error.position == NULL);
  check_pad_node(get_nth_node(&root, 0), "😀", EPT_FILL_EOL);
  expando_tree_free(&root);
}
