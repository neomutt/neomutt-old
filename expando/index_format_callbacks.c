#include <assert.h>
#include "email/lib.h"
#include "hdrline.h"
#include "index_format_callbacks.h"
#include "parser.h"

int index_C(const struct ExpandoNode *self, char *buf, int buflen, int col,
            int cols, intptr_t data, MuttFormatFlags flags)
{
  assert(self->type == NT_EXPANDO);
  const struct ExpandoExpandoNode *n = (struct ExpandoExpandoNode *) self;
  (void) n;

  struct HdrFormatInfo *hfi = (struct HdrFormatInfo *) data;
  struct Email *e = hfi->email;
  // TODO(g0mb4): format

  return snprintf(buf + col, buflen - col, "%d", e->msgno + 1);
}