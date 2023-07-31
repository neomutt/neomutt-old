#include "common.h"

void test_expando_new_if_else(void)
{
  const char *input = "if: %<l?%4l>  if-else: %<l?%4l&%4c>";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, &input, NULL, NULL, NULL, &error);

  TEST_CHECK(error.position == NULL);
  check_text_node(get_nth_node(&root, 0), "if: ");

  {
    struct ExpandoNode *n = get_nth_node(&root, 1);
    check_condition_node_head(n);
    struct ExpandoConditionNode *cond = (struct ExpandoConditionNode *) n;

    check_expando_node(cond->condition, "l", NULL);

    struct ExpandoFormat fmt = { 0 };
    fmt.min = 4;
    fmt.max = 0;
    fmt.justification = FMT_J_RIGHT;
    fmt.leader = ' ';
    check_expando_node(cond->if_true, "l", &fmt);
    TEST_CHECK(cond->if_false == NULL);
  }

  check_text_node(get_nth_node(&root, 2), "  if-else: ");

  {
    struct ExpandoNode *n = get_nth_node(&root, 3);
    check_condition_node_head(n);
    struct ExpandoConditionNode *cond = (struct ExpandoConditionNode *) n;

    check_expando_node(cond->condition, "l", NULL);

    struct ExpandoFormat fmt = { 0 };
    fmt.min = 4;
    fmt.max = 0;
    fmt.justification = FMT_J_RIGHT;
    fmt.leader = ' ';
    check_expando_node(cond->if_true, "l", &fmt);
    check_expando_node(cond->if_false, "c", &fmt);
  }

  expando_tree_free(&root);
}