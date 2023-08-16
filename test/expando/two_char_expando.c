#include "common.h"

#include "expando/validation.h"

void test_expando_two_char_expando(void)
{
  const char *input = "%cr %ab";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, &input, EFMTI_INDEX_FORMAT, &error);

  TEST_CHECK(error.position == NULL);
  check_expando_node(get_nth_node(&root, 0), "cr", NULL);
  check_text_node(get_nth_node(&root, 1), " ");
  check_expando_node(get_nth_node(&root, 2), "a", NULL);
  check_text_node(get_nth_node(&root, 3), "b");

  expando_tree_free(&root);
}