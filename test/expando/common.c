#include "common.h"

struct ExpandoNode *get_nth_node(struct ExpandoNode **root, int n)
{
  TEST_CHECK(*root != NULL);

  struct ExpandoNode *node = *root;
  int i = 0;

  while (node)
  {
    if (i++ == n)
    {
      return node;
    }

    node = node->next;
  }

  if (!TEST_CHECK(0))
  {
    TEST_MSG("Node is not found\n");
  }

  return NULL;
}

void check_empty_node(struct ExpandoNode *node)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == NT_EMPTY);
}

void check_text_node(struct ExpandoNode *node, const char *text)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == NT_TEXT);

  const int n = strlen(text);
  const int m = (int) (node->end - node->start);
  TEST_CHECK(n == m);
  TEST_CHECK(strncmp(node->start, text, n) == 0);
}

void check_expando_node(struct ExpandoNode *node, const char *expando,
                        const struct ExpandoFormatPrivate *format)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == NT_EXPANDO);

  const int n = strlen(expando);
  const int m = (int) (node->end - node->start);

  TEST_CHECK(n == m);
  TEST_CHECK(strncmp(node->start, expando, n) == 0);

  if (format == NULL)
  {
    TEST_CHECK(node->ndata == NULL);
  }
  else
  {
    struct ExpandoFormatPrivate *f = (struct ExpandoFormatPrivate *) node->ndata;
    TEST_CHECK(f != NULL);
    TEST_CHECK(f->justification == format->justification);
    TEST_CHECK(f->leader == format->leader);
    TEST_CHECK(f->min == format->min);
    TEST_CHECK(f->max == format->max);
  }
}

void check_pad_node(struct ExpandoNode *node, const char *pad_char, enum ExpandoPadType pad_type)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == NT_PAD);

  const int n = strlen(pad_char);
  const int m = (int) (node->end - node->start);

  TEST_CHECK(n == m);
  TEST_CHECK(strncmp(node->start, pad_char, n) == 0);

  struct ExpandoPadPrivate *p = (struct ExpandoPadPrivate *) node->ndata;
  TEST_CHECK(p->pad_type == pad_type);
}

void check_date_node(struct ExpandoNode *node, const char *inner_text,
                     enum ExpandoDateType date_type, bool ingnore_locale)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == NT_DATE);

  const int n = strlen(inner_text);
  const int m = (int) (node->end - node->start);
  TEST_CHECK(n == m);
  TEST_CHECK(strncmp(node->start, inner_text, n) == 0);

  struct ExpandoDatePrivate *d = (struct ExpandoDatePrivate *) node->ndata;
  TEST_CHECK(d->date_type == date_type);
  TEST_CHECK(d->ingnore_locale == ingnore_locale);
}

void check_condition_node_head(struct ExpandoNode *node)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == NT_CONDITION);
}

void check_index_hook_node(struct ExpandoNode *node, const char *name)
{
  TEST_CHECK(node != NULL);
  TEST_CHECK(node->type == NT_INDEX_FORMAT_HOOK);

  const int n = strlen(name);
  const int m = (int) (node->end - node->start);
  TEST_CHECK(n == m);
  TEST_CHECK(strncmp(node->start, name, n) == 0);
}

/* For the linker */
struct AddressList;
bool check_for_mailing_list(struct AddressList *al, const char *pfx, char *buf, int buflen)
{
  return false;
}

typedef uint8_t MuttThreadFlags;
struct Email;
int mutt_traverse_thread(struct Email *e, MuttThreadFlags flag)
{
  return 0;
}

int mutt_thread_style(void)
{
  return 0;
}

const char *mutt_get_name(const char *s)
{
  return NULL;
}

struct Envelope;
bool subjrx_apply_mods(struct Envelope *env)
{
  return false;
}

bool OptAttachMsg;

struct MailboxView;
bool mview_has_limit(const struct MailboxView *mv)
{
  return false;
}
