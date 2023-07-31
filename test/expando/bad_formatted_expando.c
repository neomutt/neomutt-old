#include "common.h"
#include "expando/helpers.h"

void test_expando_bad_formatted_expando(void)
{
  const char *input = "%.a";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, &input, NULL, NULL, NULL, &error);

  TEST_CHECK(error.position != NULL);

  expando_tree_free(&root);
}