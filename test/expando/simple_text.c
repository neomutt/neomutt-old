#include "common.h"

void test_expando_simple_text(void)
{
  const char *input = "test text";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, &input, EFMTI_FORMAT_COUNT_OR_DEBUG, &error);

  TEST_CHECK(error.position == NULL);
  check_text_node(get_nth_node(&root, 0), input);

  expando_tree_free(&root);
}
