#define TEST_NO_MAIN
#include "config.h"
#include "acutest.h"
#include "expando/parser.h"

void test_expando_percent_sign_text(void)
{
  const char *text = "percent %%";
  const char *input = text;
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, input, NULL, NULL, NULL, &error);

  TEST_CHECK(root != NULL);
  TEST_CHECK(error.position == NULL);
  TEST_CHECK(root->type == NT_TEXT);

  const struct ExpandoTextNode *n1 = (struct ExpandoTextNode *) root;

  TEST_CHECK(strncmp(n1->start, "percent ", strlen("percent ")) == 0);

  TEST_CHECK(root->next != NULL);
  TEST_CHECK(root->next->type == NT_TEXT);

  const struct ExpandoTextNode *n2 = (struct ExpandoTextNode *) root->next;

  TEST_CHECK(strncmp(n2->start, "%", 1) == 0);

  expando_tree_free(&root);
}