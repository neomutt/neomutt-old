#ifndef TEST_EXPANDO_COMMON_H
#define TEST_EXPANDO_COMMON_H

#define TEST_NO_MAIN
#include "config.h"
#include "acutest.h"
#include "expando/lib.h"

struct ExpandoNode *get_nth_node(struct ExpandoNode **root, int n);
void check_empty_node(struct ExpandoNode *node);
void check_text_node(struct ExpandoNode *node, const char *text);
void check_expando_node(struct ExpandoNode *node, const char *expando,
                        const struct ExpandoFormatPrivate *format);
void check_pad_node(struct ExpandoNode *node, const char *pad_char,
                    enum ExpandoPadType pad_type);
void check_date_node(struct ExpandoNode *node, const char *inner_text,
                     enum ExpandoDateType date_type, bool ingnore_locale);
void check_condition_node_head(struct ExpandoNode *node);
void check_index_hook_node(struct ExpandoNode *node, const char *name);

#endif /* TEST_EXPANDO_COMMON_H */
