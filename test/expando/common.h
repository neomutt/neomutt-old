#ifndef TEST_EXPANDO_COMMON_H
#define TEST_EXPANDO_COMMON_H

#define TEST_NO_MAIN
#include "config.h"
#include "acutest.h"
#include "expando/parser.h"

struct ExpandoNode *get_nth_node(struct ExpandoNode **root, int n);
void check_text_node(struct ExpandoNode *node, const char *text);
void check_expando_node(struct ExpandoNode *node, const char *expando,
                        const struct ExpandoFormat *format);
void check_pad_node(struct ExpandoNode *node, char pad_char, enum ExpandoPadType pad_type);

#endif