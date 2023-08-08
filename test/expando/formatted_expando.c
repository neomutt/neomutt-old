#include "common.h"

void test_expando_formatted_expando(void)
{
  const char *input = "%X %8X %-8X %08X %.8X %8.8X %-8.8X";
  struct ExpandoParseError error = { 0 };
  struct ExpandoNode *root = NULL;

  expando_tree_parse(&root, &input, NULL, NULL, NULL, &error);

  TEST_CHECK(error.position == NULL);
  check_expando_node(get_nth_node(&root, 0), "X", NULL);
  check_text_node(get_nth_node(&root, 1), " ");

  {
    struct ExpandoFormatPrivate fmt = { 0 };
    fmt.min = 8;
    fmt.max = 0;
    fmt.justification = JUSTIFY_RIGHT;
    fmt.leader = ' ';
    check_expando_node(get_nth_node(&root, 2), "X", &fmt);
    check_text_node(get_nth_node(&root, 3), " ");
  }

  {
    struct ExpandoFormatPrivate fmt = { 0 };
    fmt.min = 8;
    fmt.max = 0;
    fmt.justification = JUSTIFY_LEFT;
    fmt.leader = ' ';
    check_expando_node(get_nth_node(&root, 4), "X", &fmt);
    check_text_node(get_nth_node(&root, 5), " ");
  }

  {
    struct ExpandoFormatPrivate fmt = { 0 };
    fmt.min = 8;
    fmt.max = 0;
    fmt.justification = JUSTIFY_RIGHT;
    fmt.leader = '0';
    check_expando_node(get_nth_node(&root, 6), "X", &fmt);
    check_text_node(get_nth_node(&root, 7), " ");
  }

  {
    struct ExpandoFormatPrivate fmt = { 0 };
    fmt.min = 0;
    fmt.max = 8;
    fmt.justification = JUSTIFY_RIGHT;
    fmt.leader = ' ';
    check_expando_node(get_nth_node(&root, 8), "X", &fmt);
    check_text_node(get_nth_node(&root, 9), " ");
  }

  {
    struct ExpandoFormatPrivate fmt = { 0 };
    fmt.min = 8;
    fmt.max = 8;
    fmt.justification = JUSTIFY_RIGHT;
    fmt.leader = ' ';
    check_expando_node(get_nth_node(&root, 10), "X", &fmt);
    check_text_node(get_nth_node(&root, 11), " ");
  }

  {
    struct ExpandoFormatPrivate fmt = { 0 };
    fmt.min = 8;
    fmt.max = 8;
    fmt.justification = JUSTIFY_LEFT;
    fmt.leader = ' ';
    check_expando_node(get_nth_node(&root, 12), "X", &fmt);
  }

  expando_tree_free(&root);
}