#define TEST_NO_MAIN
#include "config.h"
#include "acutest.h"
#include "expando/parser.h"

void test_expando_simple_text(void)
{
  const char *text = "test text";
  const char *input = text;
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, input, NULL, NULL, NULL, &error);

  TEST_CHECK(root != NULL);
  TEST_CHECK(error.position == NULL);
  TEST_CHECK(root->type == NT_TEXT);

  const struct ExpandoTextNode *n = (struct ExpandoTextNode *) root;

  TEST_CHECK(strncmp(n->start, text, strlen(text)) == 0);

  expando_tree_free(&root);
}