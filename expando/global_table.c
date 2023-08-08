#include "mutt/memory.h"
#include "assert.h"
#include "global_table.h"
#include "node.h"

void expando_tree_free(struct ExpandoNode **root);

struct ExpandoRecord *expando_global_table_new(void)
{
  struct ExpandoRecord *t = mutt_mem_calloc(EFMT_FORMAT_COUNT, sizeof(struct ExpandoRecord));
  return t;
}

void expando_global_table_free(struct ExpandoRecord **ptr)
{
  if (!ptr || !*ptr)
    return;

  struct ExpandoRecord *table = *ptr;
  for (int i = 0; i < EFMT_FORMAT_COUNT; ++i)
  {
    struct ExpandoRecord *r = &table[i];
    if (r->tree)
    {
      expando_tree_free(&r->tree);
    }

    if (r->string)
    {
      FREE(&r->string);
    }
  }

  FREE(ptr);
}
