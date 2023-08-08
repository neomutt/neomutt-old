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

void free_expando_private(void **ptr)
{
  FREE(ptr);
}

void free_expando_private_condition_node(void **ptr)
{
  struct ExpandoConditionPrivate *p = *ptr;

  if (p->condition)
  {
    free_node(p->condition);
  }

  if (p->if_true)
  {
    free_node(p->if_true);
  }

  if (p->if_false)
  {
    free_node(p->if_false);
  }

  FREE(ptr);
}