#include "common.h"

void test_expando_date(void)
{
  const char *input = "%[%b %d] %{!%b %d} %(%b %d)";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, &input, EFMTI_FORMAT_COUNT_OR_DEBUG, &error);

  TEST_CHECK(error.position == NULL);
  check_date_node(get_nth_node(&root, 0), "%b %d", EDT_LOCAL_SEND_TIME, false);
  check_text_node(get_nth_node(&root, 1), " ");
  check_date_node(get_nth_node(&root, 2), "%b %d", EDT_SENDER_SEND_TIME, true);
  check_text_node(get_nth_node(&root, 3), " ");
  check_date_node(get_nth_node(&root, 4), "%b %d", EDT_LOCAL_RECIEVE_TIME, false);

  expando_tree_free(&root);
}