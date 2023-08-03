#include <assert.h>
#include "email/lib.h"
#include "hdrline.h"
#include "helpers.h"
#include "index_format_callbacks.h"
#include "parser.h"

void index_C(const struct ExpandoNode *self, char **buffer, int *buffer_len,
             int *start_col, int max_cols, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  const struct ExpandoExpandoNode *n = (struct ExpandoExpandoNode *) self;
  (void) n;

  struct HdrFormatInfo *hfi = (struct HdrFormatInfo *) data;
  struct Email *e = hfi->email;
  // TODO(g0mb4): format

  int printed = snprintf(*buffer, *buffer_len, "%d", e->msgno + 1);

  *start_col = mb_strwidth_range(*buffer, *buffer + printed);
  *buffer_len -= printed;
  *buffer += printed;
}