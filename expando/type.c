/**
 * @file
 * XXX
 *
 * @authors
 * Copyright (C) 2023 Tóth János <gomba007@gmail.com>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @page config_expandos Type: Expando
 *
 */

#include "config.h"
#include <stddef.h>
#include <stdint.h>
#include "mutt/lib.h"
#include "config/lib.h"
#include "type.h"
#include "assert.h"
#include "parser.h"

/**
 * expando_destroy - Destroy an ExpandoRecord object - Implements ConfigSetType::destroy() - @ingroup cfg_type_destroy
 */
static void expando_destroy(const struct ConfigSet *cs, void *var, const struct ConfigDef *cdef)
{
  struct ExpandoRecord **a = var;
  if (!*a)
    return;

  expando_free(a);
}

/**
 * expando_string_set - Set an ExpandoRecord by string - Implements ConfigSetType::string_set() - @ingroup cfg_type_string_set
 */
static int expando_string_set(const struct ConfigSet *cs, void *var, struct ConfigDef *cdef,
                              const char *value, struct Buffer *err)
{
  assert(cdef->validator);
  struct ExpandoRecord *r = expando_new(value);

  int rc = CSR_SUCCESS;

  if (var)
  {
    rc = cdef->validator(cs, cdef, (intptr_t) r, err);

    if (CSR_RESULT(rc) != CSR_SUCCESS)
    {
      expando_destroy(cs, &r, cdef);
      return rc | CSR_INV_VALIDATOR;
    }

    /* ordinary variable setting */
    expando_destroy(cs, var, cdef);

    *(struct ExpandoRecord **) var = r;

    if (!r)
      rc |= CSR_SUC_EMPTY;
  }
  else
  {
    /* set the default/initial value */
    if (cdef->type & DT_INITIAL_SET)
      FREE(&cdef->initial);

    cdef->type |= DT_INITIAL_SET;
    cdef->initial = (intptr_t) mutt_str_dup(value);
  }

  return rc;
}

/**
 * expando_string_get - Get an ExpandoRecord as a string - Implements ConfigSetType::string_get() - @ingroup cfg_type_string_get
 */
static int expando_string_get(const struct ConfigSet *cs, void *var,
                              const struct ConfigDef *cdef, struct Buffer *result)
{
  if (var)
  {
    struct ExpandoRecord *r = *(struct ExpandoRecord **) var;
    if (r)
    {
      buf_addstr(result, r->string);
    }
  }
  else
  {
    buf_addstr(result, (char *) cdef->initial);
  }

  if (buf_is_empty(result))
    return CSR_SUCCESS | CSR_SUC_EMPTY; /* empty string */

  return CSR_SUCCESS;
}

/**
 * expando_native_set - Set an ExpandoRecord object from an ExpandoRecord config item - Implements ConfigSetType::native_get() - @ingroup cfg_type_native_get
 */
static int expando_native_set(const struct ConfigSet *cs, void *var,
                              const struct ConfigDef *cdef, intptr_t value,
                              struct Buffer *err)
{
  assert(0 && "Unreachable");
  return CSR_ERR_CODE;
}

/**
 * expando_native_get - Get an ExpandoRecord object from an ExpandoRecord config item - Implements ConfigSetType::native_get() - @ingroup cfg_type_native_get
 */
static intptr_t expando_native_get(const struct ConfigSet *cs, void *var,
                                   const struct ConfigDef *cdef, struct Buffer *err)
{
  struct ExpandoRecord *r = *(struct ExpandoRecord **) var;
  return (intptr_t) r;
}

/**
 * expando_reset - Reset an ExpandoRecord to its initial value - Implements ConfigSetType::reset() - @ingroup cfg_type_reset
 */
static int expando_reset(const struct ConfigSet *cs, void *var,
                         const struct ConfigDef *cdef, struct Buffer *err)
{
  assert(cdef->validator);

  const char *initial = NONULL((const char *) cdef->initial);

  struct ExpandoRecord *r = expando_new(initial);

  int rc = cdef->validator(cs, cdef, (intptr_t) r, err);

  if (CSR_RESULT(rc) != CSR_SUCCESS)
  {
    expando_destroy(cs, &r, cdef);
    return rc | CSR_INV_VALIDATOR;
  }

  if (!r)
    rc |= CSR_SUC_EMPTY;

  expando_destroy(cs, var, cdef);

  *(struct ExpandoRecord **) var = r;
  return rc;
}

/**
 * expando_new - Create an ExpandoRecord from a string
 * @param addr Format string to parse
 * @retval ptr New ExpandoRecord object
 */
struct ExpandoRecord *expando_new(const char *format)
{
  struct ExpandoRecord *r = mutt_mem_calloc(1, sizeof(struct ExpandoRecord));
  r->string = mutt_str_dup(format);
  return r;
}

/**
 * expando_free - Free an ExpandoRecord object
 * @param[out] ptr ExpandoRecord to free
 */
void expando_free(struct ExpandoRecord **ptr)
{
  if (!ptr || !*ptr)
    return;

  struct ExpandoRecord *r = *ptr;
  if (r->tree)
  {
    expando_tree_free(&r->tree);
  }

  if (r->string)
  {
    FREE(&r->string);
  }

  FREE(ptr);
}

/**
 * CstExpando - Config type representing an ExpandoRecord
 */
const struct ConfigSetType CstExpando = {
  DT_EXPANDO,
  "expando",
  expando_string_set,
  expando_string_get,
  expando_native_set,
  expando_native_get,
  NULL, // string_plus_equals
  NULL, // string_minus_equals
  expando_reset,
  expando_destroy,
};
