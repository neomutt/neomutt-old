#include "common.h"

void test_expando_two_char_expando(void)
{
  const char *text = "%aa %ab";
  const char *input = text;
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  const char *valid_two_char_expandos[] = { "aa", NULL };

  expando_tree_parse(&root, input, NULL, valid_two_char_expandos, NULL, &error);

  TEST_CHECK(error.position == NULL);
  check_expando_node(get_nth_node(&root, 0), "aa", NULL);
  check_text_node(get_nth_node(&root, 1), " ");
  check_expando_node(get_nth_node(&root, 2), "a", NULL);
  check_text_node(get_nth_node(&root, 3), "b");

  expando_tree_free(&root);
}