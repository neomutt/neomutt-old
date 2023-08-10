#include "node.h"

#include "mutt/memory.h"

void free_node(struct ExpandoNode *node)
{
  if (node == NULL)
  {
    return;
  }

  if (node->ndata_free)
  {
    node->ndata_free(&node->ndata);
  }

  FREE(&node);
}

void free_tree(struct ExpandoNode *node)
{
  while (node)
  {
    struct ExpandoNode *n = node;
    node = node->next;
    free_node(n);
  }
}

void free_expando_private(void **ptr)
{
  FREE(ptr);
}

void free_expando_private_condition_node(void **ptr)
{
  struct ExpandoConditionPrivate *p = *ptr;

  free_node(p->condition);
  free_tree(p->if_true_tree);
  free_tree(p->if_false_tree);

  FREE(ptr);
}