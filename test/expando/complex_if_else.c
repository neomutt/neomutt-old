#include "common.h"
#include "limits.h"

void test_expando_complex_if_else(void)
{
  const char *input = "if: %<l?pre %4lpost> if-else: %<l?pre %4lpost&pre %4cpost>";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, &input, EFMTI_FORMAT_COUNT_OR_DEBUG, &error);

  TEST_CHECK(error.position == NULL);
  check_text_node(get_nth_node(&root, 0), "if: ");

  {
    struct ExpandoNode *n = get_nth_node(&root, 1);
    check_condition_node_head(n);
    struct ExpandoConditionPrivate *cond = (struct ExpandoConditionPrivate *) n->ndata;

    check_expando_node(cond->condition, "l", NULL);

    struct ExpandoFormatPrivate fmt = { 0 };
    fmt.min = 4;
    fmt.max = INT_MAX;
    fmt.justification = JUSTIFY_RIGHT;
    fmt.leader = ' ';

    check_text_node(get_nth_node(&cond->if_true_tree, 0), "pre ");
    check_expando_node(get_nth_node(&cond->if_true_tree, 1), "l", &fmt);
    check_text_node(get_nth_node(&cond->if_true_tree, 2), "post");

    TEST_CHECK(cond->if_false_tree == NULL);
  }

  check_text_node(get_nth_node(&root, 2), " if-else: ");

  {
    struct ExpandoNode *n = get_nth_node(&root, 3);
    check_condition_node_head(n);
    struct ExpandoConditionPrivate *cond = (struct ExpandoConditionPrivate *) n->ndata;

    check_expando_node(cond->condition, "l", NULL);

    struct ExpandoFormatPrivate fmt = { 0 };
    fmt.min = 4;
    fmt.max = INT_MAX;
    fmt.justification = JUSTIFY_RIGHT;
    fmt.leader = ' ';

    check_text_node(get_nth_node(&cond->if_true_tree, 0), "pre ");
    check_expando_node(get_nth_node(&cond->if_true_tree, 1), "l", &fmt);
    check_text_node(get_nth_node(&cond->if_true_tree, 2), "post");

    check_text_node(get_nth_node(&cond->if_false_tree, 0), "pre ");
    check_expando_node(get_nth_node(&cond->if_false_tree, 1), "c", &fmt);
    check_text_node(get_nth_node(&cond->if_false_tree, 2), "post");
  }

  expando_tree_free(&root);
}
