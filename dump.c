#include "config.h"
#include <stdio.h>
#include "mutt/mutt.h"
#include "body.h"
#include "envelope.h"
#include "header.h"
#include "dump.h"

void dump_parameter(struct ParameterList *pl, int i)
{
  struct Parameter *np;
  TAILQ_FOREACH(np, pl, entries)
  {
    printf("%*s%s : %s\n", i, "", np->attribute, np->value);
  }
}

void dump_body(struct Body *b, int i)
{
  if (b->xtype)       printf("%*sxtype: %s\n", i, "", b->xtype);
  if (b->subtype)     printf("%*ssubtype: %s\n", i, "", b->subtype);
  if (b->language)    printf("%*slanguage: %s\n", i, "", b->language);
  if (b->description) printf("%*sdescription: %s\n", i, "", b->description);
  if (b->filename)    printf("%*sfilename: %s\n", i, "", b->filename);
  if (b->d_filename)  printf("%*sd_filename: %s\n", i, "", b->d_filename);

  if (!TAILQ_EMPTY(&b->parameter))
  {
    printf("%*sparameter\n", i, "");
    dump_parameter(&b->parameter, i + 4);
  }
  if (b->hdr)
  {
    printf("%*shdr: %p\n", i, "", (void *) b->hdr);
    dump_header(b->hdr, i + 4);
  }
  if (b->next)
  {
    printf("%*snext: %p\n", i, "", (void *) b->next);
    dump_body(b->next, i + 4);
  }
  if (b->parts)
  {
    printf("%*sparts: %p\n", i, "", (void *) b->parts);
    dump_body(b->parts, i + 4);
  }
}

void dump_address(const char *label, struct Address *addr, int i)
{
  if (!addr)
    return;
  if (addr->personal)
    printf("%*s%s: %s <%s>\n", i, "", label, addr->personal, addr->mailbox);
  else
    printf("%*s%s: %s\n", i, "", label, addr->mailbox);
}

void dump_envelope(struct Envelope *env, int i)
{
  dump_address("return_path", env->return_path, i);
  dump_address("from", env->from, i);
  dump_address("to", env->to, i);
  dump_address("cc", env->cc, i);
  dump_address("bcc", env->bcc, i);
  dump_address("sender", env->sender, i);
  dump_address("reply_to", env->reply_to, i);
  dump_address("mail_followup_to", env->mail_followup_to, i);
  dump_address("x_original_to", env->x_original_to, i);
  if (env->subject) printf("%*ssubject: %s\n", i, "", env->subject);
}

void dump_header(struct Header *h, int i)
{
  if (h->env)
  {
    printf("%*senv: %p\n", i, "", (void *) h->env);
    dump_envelope(h->env, i + 4);
  }
  if (h->content)
  {
    printf("%*scontent: %p\n", i, "", (void *) h->content);
    dump_body(h->content, i + 4);
  }
}

#if 0
char *form_name
long hdr_offset
LOFF_T offset
LOFF_T length
char *charset
struct AttachPtr *aptr
signed short attach_count
time_t stamp
unsigned int type
unsigned int encoding
unsigned int disposition
bool use_disp
bool unlink
bool tagged
bool deleted
bool noconv
bool force_charset
bool is_signed_data
bool goodsig
bool warnsig
bool badsig
bool collapsed
bool attach_qualifies
#endif
