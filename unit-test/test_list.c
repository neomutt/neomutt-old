#include "mutt.h"
#include "lib.h"

void test_list_create()
{
  struct List *l = mutt_new_list();
  TEST_CHECK(l != NULL);
}

void test_list_add()
{
  struct List *l1 = mutt_new_list();
  struct List *l2 = mutt_add_list(l1, "Hello");
  TEST_CHECK(l1 == l2);
}

void test_list_find()
{
  struct List *l1 = mutt_new_list();
  struct List *l2 = mutt_find_list(l1, "Hello");
  TEST_CHECK(l2 == NULL);

  mutt_add_list(l1, "Hello");
  l2 = mutt_find_list(l1, "Hello");
  TEST_CHECK(l1 != l2);
  TEST_CHECK(strcmp(l2->data, "Hello") == 0);
}
