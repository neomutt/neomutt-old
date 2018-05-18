#ifndef _MUTT_DUMP_H
#define _MUTT_DUMP_H

#include <stdio.h>
#include <stdlib.h>

struct Body;
struct Envelope;
struct Header;
struct ParameterList;

void dump_body(struct Body *b, int i);
void dump_envelope(struct Envelope *env, int i);
void dump_header(struct Header *h, int i);
void dump_parameter(struct ParameterList *pl, int i);

#endif /* _MUTT_DUMP_H */

