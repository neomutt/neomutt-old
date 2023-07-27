#define TEST_NO_MAIN
#include "common.h"

void test_expando_emoji_text(void)
{
  const char *text = "emoji text😀😀😀😀";
  const char *input = text;
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, input, NULL, NULL, NULL, &error);

  TEST_CHECK(error.position == NULL);
  check_text_node(get_nth_node(&root, 0), text);

  expando_tree_free(&root);
}