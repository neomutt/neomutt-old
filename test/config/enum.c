/**
 * @file
 * Test code for the Enum object
 *
 * @authors
 * Copyright (C) 2018 Richard Russon <rich@flatcap.org>
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

#define TEST_NO_MAIN
#include "config.h"
#include "acutest.h"
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "mutt/lib.h"
#include "config/common.h"
#include "config/lib.h"
#include "core/lib.h"
#include "test_common.h"

// clang-format off
enum AnimalType
{
  ANIMAL_ANTELOPE  =  1,
  ANIMAL_BADGER    =  2,
  ANIMAL_CASSOWARY =  3,
  ANIMAL_DINGO     = 40,
  ANIMAL_ECHIDNA   = 41,
  ANIMAL_FROG      = 42,
};

static struct Mapping AnimalMap[] = {
  { "Antelope",  ANIMAL_ANTELOPE,  },
  { "Badger",    ANIMAL_BADGER,    },
  { "Cassowary", ANIMAL_CASSOWARY, },
  { "Dingo",     ANIMAL_DINGO,     },
  { "Echidna",   ANIMAL_ECHIDNA,   },
  { "Frog",      ANIMAL_FROG,      },
  // Alternatives
  { "bird",      ANIMAL_CASSOWARY, },
  { "amphibian", ANIMAL_FROG,      },
  { "carnivore", ANIMAL_BADGER,    },
  { "herbivore", ANIMAL_ANTELOPE,  },
  { NULL,        0,                },
};

static struct EnumDef AnimalDef = {
  "animal",
  5,
  (struct Mapping *) &AnimalMap,
};

static struct ConfigDef Vars[] = {
  { "Apple",      DT_ENUM, ANIMAL_DINGO,    IP &AnimalDef, NULL,              }, /* test_initial_values */
  { "Banana",     DT_ENUM, ANIMAL_BADGER,   IP &AnimalDef, NULL,              },
  { "Cherry",     DT_ENUM, ANIMAL_FROG,     IP &AnimalDef, NULL,              },
  { "Damson",     DT_ENUM, ANIMAL_ANTELOPE, IP &AnimalDef, NULL,              }, /* test_string_set */
  { "Elderberry", DT_ENUM, ANIMAL_ANTELOPE, 0,             NULL,              }, /* broken */
  { "Fig",        DT_ENUM, ANIMAL_ANTELOPE, IP &AnimalDef, NULL,              }, /* test_string_get */
  { "Guava",      DT_ENUM, ANIMAL_ANTELOPE, IP &AnimalDef, NULL,              }, /* test_native_set */
  { "Hawthorn",   DT_ENUM, ANIMAL_ANTELOPE, IP &AnimalDef, NULL,              },
  { "Ilama",      DT_ENUM, ANIMAL_ANTELOPE, IP &AnimalDef, NULL,              }, /* test_native_get */
  { "Jackfruit",  DT_ENUM, ANIMAL_ANTELOPE, IP &AnimalDef, NULL,              }, /* test_reset */
  { "Kumquat",    DT_ENUM, ANIMAL_ANTELOPE, IP &AnimalDef, validator_fail,    },
  { "Lemon",      DT_ENUM, ANIMAL_ANTELOPE, IP &AnimalDef, validator_succeed, }, /* test_validator */
  { "Mango",      DT_ENUM, ANIMAL_ANTELOPE, IP &AnimalDef, validator_warn,    },
  { "Nectarine",  DT_ENUM, ANIMAL_ANTELOPE, IP &AnimalDef, validator_fail,    },
  { "Olive",      DT_ENUM, ANIMAL_ANTELOPE, IP &AnimalDef, NULL,              }, /* test_inherit */
  { NULL },
};
// clang-format on

static bool test_initial_values(struct ConfigSubset *sub, struct Buffer *err)
{
  log_line(__func__);
  struct ConfigSet *cs = sub->cs;

  unsigned char VarApple = cs_subset_enum(sub, "Apple");
  unsigned char VarBanana = cs_subset_enum(sub, "Banana");

  TEST_MSG("Apple = %d\n", VarApple);
  TEST_MSG("Banana = %d\n", VarBanana);

  if (!TEST_CHECK(VarApple == ANIMAL_DINGO))
  {
    TEST_MSG("Expected: %d\n", ANIMAL_DINGO);
    TEST_MSG("Actual  : %d\n", VarApple);
  }

  if (!TEST_CHECK(VarBanana == ANIMAL_BADGER))
  {
    TEST_MSG("Expected: %d\n", ANIMAL_BADGER);
    TEST_MSG("Actual  : %d\n", VarBanana);
  }

  cs_str_string_set(cs, "Apple", "Cassowary", err);
  cs_str_string_set(cs, "Banana", "herbivore", err);

  VarApple = cs_subset_enum(sub, "Apple");
  VarBanana = cs_subset_enum(sub, "Banana");

  struct Buffer *value = mutt_buffer_pool_get();

  int rc;

  mutt_buffer_reset(value);
  rc = cs_str_initial_get(cs, "Apple", value);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("%s\n", mutt_buffer_string(value));
    return false;
  }

  if (!TEST_CHECK(mutt_str_equal(mutt_buffer_string(value), "Dingo")))
  {
    TEST_MSG("Apple's initial value is wrong: '%s'\n", mutt_buffer_string(value));
    return false;
  }
  VarApple = cs_subset_enum(sub, "Apple");
  TEST_MSG("Apple = %d\n", VarApple);
  TEST_MSG("Apple's initial value is '%s'\n", mutt_buffer_string(value));

  mutt_buffer_reset(value);
  rc = cs_str_initial_get(cs, "Banana", value);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("%s\n", mutt_buffer_string(value));
    return false;
  }

  if (!TEST_CHECK(mutt_str_equal(mutt_buffer_string(value), "Badger")))
  {
    TEST_MSG("Banana's initial value is wrong: '%s'\n", mutt_buffer_string(value));
    return false;
  }
  VarBanana = cs_subset_enum(sub, "Banana");
  TEST_MSG("Banana = %d\n", VarBanana);
  TEST_MSG("Banana's initial value is '%s'\n", NONULL(mutt_buffer_string(value)));

  mutt_buffer_reset(value);
  rc = cs_str_initial_set(cs, "Cherry", "bird", value);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("%s\n", mutt_buffer_string(value));
    return false;
  }

  mutt_buffer_reset(value);
  rc = cs_str_initial_get(cs, "Cherry", value);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("%s\n", mutt_buffer_string(value));
    return false;
  }

  unsigned char VarCherry = cs_subset_enum(sub, "Cherry");
  TEST_MSG("Cherry = %d\n", VarCherry);
  TEST_MSG("Cherry's initial value is %s\n", mutt_buffer_string(value));

  mutt_buffer_pool_release(&value);
  log_line(__func__);
  return true;
}

static bool test_string_set(struct ConfigSubset *sub, struct Buffer *err)
{
  log_line(__func__);
  struct ConfigSet *cs = sub->cs;
  const char *valid[] = { "Antelope", "ECHIDNA", "herbivore", "BIRD" };
  int numbers[] = { 1, 41, 1, 3 };
  const char *invalid[] = { "Frogs", "", NULL };
  const char *name = "Damson";

  int rc;
  unsigned char VarDamson;
  for (unsigned int i = 0; i < mutt_array_size(valid); i++)
  {
    cs_str_native_set(cs, name, ANIMAL_CASSOWARY, NULL);

    TEST_MSG("Setting %s to %s\n", name, valid[i]);
    mutt_buffer_reset(err);
    rc = cs_str_string_set(cs, name, valid[i], err);
    if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
    {
      TEST_MSG("%s\n", mutt_buffer_string(err));
      return false;
    }

    if (rc & CSR_SUC_NO_CHANGE)
    {
      TEST_MSG("Value of %s wasn't changed\n", name);
      continue;
    }

    VarDamson = cs_subset_enum(sub, "Damson");
    if (!TEST_CHECK(VarDamson == numbers[i]))
    {
      TEST_MSG("Value of %s wasn't changed\n", name);
      return false;
    }
    TEST_MSG("%s = %d, set by '%s'\n", name, VarDamson, valid[i]);
    short_line();
  }

  for (unsigned int i = 0; i < mutt_array_size(invalid); i++)
  {
    TEST_MSG("Setting %s to %s\n", name, NONULL(invalid[i]));
    mutt_buffer_reset(err);
    rc = cs_str_string_set(cs, name, invalid[i], err);
    if (TEST_CHECK(CSR_RESULT(rc) != CSR_SUCCESS))
    {
      TEST_MSG("Expected error: %s\n", mutt_buffer_string(err));
    }
    else
    {
      VarDamson = cs_subset_enum(sub, "Damson");
      TEST_MSG("%s = %d, set by '%s'\n", name, VarDamson, invalid[i]);
      TEST_MSG("This test should have failed\n");
      return false;
    }
    short_line();
  }

  name = "Elderberry";
  const char *value2 = "Dingo";
  short_line();
  TEST_MSG("Setting %s to '%s'\n", name, value2);
  rc = cs_str_string_set(cs, name, value2, err);
  if (TEST_CHECK(CSR_RESULT(rc) != CSR_SUCCESS))
  {
    TEST_MSG("Expected error: %s\n", mutt_buffer_string(err));
  }
  else
  {
    TEST_MSG("This test should have failed\n");
    return false;
  }

  log_line(__func__);
  return true;
}

static bool test_string_get(struct ConfigSubset *sub, struct Buffer *err)
{
  log_line(__func__);
  struct ConfigSet *cs = sub->cs;
  const char *name = "Fig";

  cs_str_native_set(cs, name, ANIMAL_ECHIDNA, NULL);
  mutt_buffer_reset(err);
  int rc = cs_str_string_get(cs, name, err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("Get failed: %s\n", mutt_buffer_string(err));
    return false;
  }
  unsigned char VarFig = cs_subset_enum(sub, "Fig");
  TEST_MSG("%s = %d, %s\n", name, VarFig, mutt_buffer_string(err));

  cs_str_native_set(cs, name, ANIMAL_DINGO, NULL);
  mutt_buffer_reset(err);
  rc = cs_str_string_get(cs, name, err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("Get failed: %s\n", mutt_buffer_string(err));
    return false;
  }
  VarFig = cs_subset_enum(sub, "Fig");
  TEST_MSG("%s = %d, %s\n", name, VarFig, mutt_buffer_string(err));

  log_line(__func__);
  return true;
}

static bool test_native_set(struct ConfigSubset *sub, struct Buffer *err)
{
  log_line(__func__);
  struct ConfigSet *cs = sub->cs;
  const char *name = "Guava";
  unsigned char value = ANIMAL_CASSOWARY;

  TEST_MSG("Setting %s to %d\n", name, value);
  cs_str_native_set(cs, name, 0, NULL);
  mutt_buffer_reset(err);
  int rc = cs_str_native_set(cs, name, value, err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("%s\n", mutt_buffer_string(err));
    return false;
  }

  unsigned char VarGuava = cs_subset_enum(sub, "Guava");
  if (!TEST_CHECK(VarGuava == value))
  {
    TEST_MSG("Value of %s wasn't changed\n", name);
    return false;
  }

  TEST_MSG("%s = %d, set to '%d'\n", name, VarGuava, value);

  short_line();
  TEST_MSG("Setting %s to %d\n", name, value);
  rc = cs_str_native_set(cs, name, value, err);
  if (TEST_CHECK((rc & CSR_SUC_NO_CHANGE) != 0))
  {
    TEST_MSG("Value of %s wasn't changed\n", name);
  }
  else
  {
    TEST_MSG("This test should have failed\n");
    return false;
  }

  name = "Hawthorn";
  value = -42;
  short_line();
  TEST_MSG("Setting %s to %d\n", name, value);
  rc = cs_str_native_set(cs, name, value, err);
  if (TEST_CHECK(CSR_RESULT(rc) != CSR_SUCCESS))
  {
    TEST_MSG("Expected error: %s\n", mutt_buffer_string(err));
  }
  else
  {
    TEST_MSG("This test should have failed\n");
    return false;
  }

  int invalid[] = { -1, 256 };
  for (unsigned int i = 0; i < mutt_array_size(invalid); i++)
  {
    short_line();
    cs_str_native_set(cs, name, ANIMAL_CASSOWARY, NULL);
    TEST_MSG("Setting %s to %d\n", name, invalid[i]);
    mutt_buffer_reset(err);
    rc = cs_str_native_set(cs, name, invalid[i], err);
    if (TEST_CHECK(CSR_RESULT(rc) != CSR_SUCCESS))
    {
      TEST_MSG("Expected error: %s\n", mutt_buffer_string(err));
    }
    else
    {
      VarGuava = cs_subset_enum(sub, "Guava");
      TEST_MSG("%s = %d, set by '%d'\n", name, VarGuava, invalid[i]);
      TEST_MSG("This test should have failed\n");
      return false;
    }
  }

  name = "Elderberry";
  value = ANIMAL_ANTELOPE;
  short_line();
  TEST_MSG("Setting %s to %d\n", name, value);
  rc = cs_str_native_set(cs, name, value, err);
  if (TEST_CHECK(CSR_RESULT(rc) != CSR_SUCCESS))
  {
    TEST_MSG("Expected error: %s\n", mutt_buffer_string(err));
  }
  else
  {
    TEST_MSG("This test should have failed\n");
    return false;
  }

  log_line(__func__);
  return true;
}

static bool test_native_get(struct ConfigSubset *sub, struct Buffer *err)
{
  log_line(__func__);
  struct ConfigSet *cs = sub->cs;
  const char *name = "Ilama";

  cs_str_native_set(cs, name, 253, NULL);
  mutt_buffer_reset(err);
  intptr_t value = cs_str_native_get(cs, name, err);
  if (!TEST_CHECK(value != INT_MIN))
  {
    TEST_MSG("Get failed: %s\n", mutt_buffer_string(err));
    return false;
  }
  TEST_MSG("%s = %ld\n", name, value);

  log_line(__func__);
  return true;
}

static bool test_reset(struct ConfigSubset *sub, struct Buffer *err)
{
  log_line(__func__);
  struct ConfigSet *cs = sub->cs;
  const char *name = "Jackfruit";
  cs_str_native_set(cs, name, 253, NULL);
  mutt_buffer_reset(err);

  unsigned char VarJackfruit = cs_subset_enum(sub, "Jackfruit");
  TEST_MSG("%s = %d\n", name, VarJackfruit);
  int rc = cs_str_reset(cs, name, err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("%s\n", mutt_buffer_string(err));
    return false;
  }

  VarJackfruit = cs_subset_enum(sub, "Jackfruit");
  if (!TEST_CHECK(VarJackfruit != 253))
  {
    TEST_MSG("Value of %s wasn't changed\n", name);
    return false;
  }

  TEST_MSG("Reset: %s = %d\n", name, VarJackfruit);

  short_line();
  name = "Kumquat";
  mutt_buffer_reset(err);

  unsigned char VarKumquat = cs_subset_enum(sub, "Kumquat");
  TEST_MSG("Initial: %s = %d\n", name, VarKumquat);
  dont_fail = true;
  rc = cs_str_string_set(cs, name, "Dingo", err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
    return false;
  VarKumquat = cs_subset_enum(sub, "Kumquat");
  TEST_MSG("Set: %s = %d\n", name, VarKumquat);
  dont_fail = false;

  rc = cs_str_reset(cs, name, err);
  if (TEST_CHECK(CSR_RESULT(rc) != CSR_SUCCESS))
  {
    TEST_MSG("Expected error: %s\n", mutt_buffer_string(err));
  }
  else
  {
    TEST_MSG("%s\n", mutt_buffer_string(err));
    return false;
  }

  VarKumquat = cs_subset_enum(sub, "Kumquat");
  if (!TEST_CHECK(VarKumquat == ANIMAL_DINGO))
  {
    TEST_MSG("Value of %s changed\n", name);
    return false;
  }

  TEST_MSG("Reset: %s = %d\n", name, VarKumquat);

  short_line();
  name = "Jackfruit";
  cs_str_native_set(cs, name, ANIMAL_ANTELOPE, NULL);
  mutt_buffer_reset(err);

  rc = cs_str_reset(cs, name, err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("%s\n", mutt_buffer_string(err));
    return false;
  }

  log_line(__func__);
  return true;
}

static bool test_validator(struct ConfigSubset *sub, struct Buffer *err)
{
  log_line(__func__);
  struct ConfigSet *cs = sub->cs;

  const char *name = "Lemon";
  cs_str_native_set(cs, name, ANIMAL_ANTELOPE, NULL);
  mutt_buffer_reset(err);
  int rc = cs_str_string_set(cs, name, "Dingo", err);
  if (TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("%s\n", mutt_buffer_string(err));
  }
  else
  {
    TEST_MSG("%s\n", mutt_buffer_string(err));
    return false;
  }
  unsigned char VarLemon = cs_subset_enum(sub, "Lemon");
  TEST_MSG("String: %s = %d\n", name, VarLemon);
  short_line();

  cs_str_native_set(cs, name, 253, NULL);
  mutt_buffer_reset(err);
  rc = cs_str_native_set(cs, name, ANIMAL_ECHIDNA, err);
  if (TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("%s\n", mutt_buffer_string(err));
  }
  else
  {
    TEST_MSG("%s\n", mutt_buffer_string(err));
    return false;
  }
  VarLemon = cs_subset_enum(sub, "Lemon");
  TEST_MSG("Native: %s = %d\n", name, VarLemon);
  short_line();

  name = "Mango";
  cs_str_native_set(cs, name, 123, NULL);
  mutt_buffer_reset(err);
  rc = cs_str_string_set(cs, name, "bird", err);
  if (TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("%s\n", mutt_buffer_string(err));
  }
  else
  {
    TEST_MSG("%s\n", mutt_buffer_string(err));
    return false;
  }
  unsigned char VarMango = cs_subset_enum(sub, "Mango");
  TEST_MSG("String: %s = %d\n", name, VarMango);
  short_line();

  cs_str_native_set(cs, name, 253, NULL);
  mutt_buffer_reset(err);
  rc = cs_str_native_set(cs, name, ANIMAL_DINGO, err);
  if (TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("%s\n", mutt_buffer_string(err));
  }
  else
  {
    TEST_MSG("%s\n", mutt_buffer_string(err));
    return false;
  }
  VarMango = cs_subset_enum(sub, "Mango");
  TEST_MSG("Native: %s = %d\n", name, VarMango);
  short_line();

  name = "Nectarine";
  cs_str_native_set(cs, name, 123, NULL);
  mutt_buffer_reset(err);
  rc = cs_str_string_set(cs, name, "Cassowary", err);
  if (TEST_CHECK(CSR_RESULT(rc) != CSR_SUCCESS))
  {
    TEST_MSG("Expected error: %s\n", mutt_buffer_string(err));
  }
  else
  {
    TEST_MSG("%s\n", mutt_buffer_string(err));
    return false;
  }
  unsigned char VarNectarine = cs_subset_enum(sub, "Nectarine");
  TEST_MSG("String: %s = %d\n", name, VarNectarine);
  short_line();

  cs_str_native_set(cs, name, 253, NULL);
  mutt_buffer_reset(err);
  rc = cs_str_native_set(cs, name, ANIMAL_CASSOWARY, err);
  if (TEST_CHECK(CSR_RESULT(rc) != CSR_SUCCESS))
  {
    TEST_MSG("Expected error: %s\n", mutt_buffer_string(err));
  }
  else
  {
    TEST_MSG("%s\n", mutt_buffer_string(err));
    return false;
  }
  VarNectarine = cs_subset_enum(sub, "Nectarine");
  TEST_MSG("Native: %s = %d\n", name, VarNectarine);

  log_line(__func__);
  return true;
}

static void dump_native(struct ConfigSet *cs, const char *parent, const char *child)
{
  intptr_t pval = cs_str_native_get(cs, parent, NULL);
  intptr_t cval = cs_str_native_get(cs, child, NULL);

  TEST_MSG("%15s = %ld\n", parent, pval);
  TEST_MSG("%15s = %ld\n", child, cval);
}

static bool test_inherit(struct ConfigSet *cs, struct Buffer *err)
{
  log_line(__func__);
  bool result = false;

  const char *account = "fruit";
  const char *parent = "Olive";
  char child[128];
  snprintf(child, sizeof(child), "%s:%s", account, parent);

  struct ConfigSubset *sub = cs_subset_new(NULL, NULL, NeoMutt->notify);
  sub->cs = cs;
  struct Account *a = account_new(account, sub);

  struct HashElem *he = cs_subset_create_inheritance(a->sub, parent);
  if (!he)
  {
    TEST_MSG("Error: %s\n", mutt_buffer_string(err));
    goto ti_out;
  }

  // set parent
  cs_str_native_set(cs, parent, ANIMAL_BADGER, NULL);
  mutt_buffer_reset(err);
  int rc = cs_str_string_set(cs, parent, "Dingo", err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("Error: %s\n", mutt_buffer_string(err));
    goto ti_out;
  }
  dump_native(cs, parent, child);
  short_line();

  // set child
  mutt_buffer_reset(err);
  rc = cs_str_string_set(cs, child, "Cassowary", err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("Error: %s\n", mutt_buffer_string(err));
    goto ti_out;
  }
  dump_native(cs, parent, child);
  short_line();

  // reset child
  mutt_buffer_reset(err);
  rc = cs_str_reset(cs, child, err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("Error: %s\n", mutt_buffer_string(err));
    goto ti_out;
  }
  dump_native(cs, parent, child);
  short_line();

  // reset parent
  mutt_buffer_reset(err);
  rc = cs_str_reset(cs, parent, err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("Error: %s\n", mutt_buffer_string(err));
    goto ti_out;
  }
  dump_native(cs, parent, child);

  log_line(__func__);
  result = true;
ti_out:
  account_free(&a);
  cs_subset_free(&sub);
  return result;
}

void test_config_enum(void)
{
  NeoMutt = test_neomutt_create();
  struct ConfigSubset *sub = NeoMutt->sub;
  struct ConfigSet *cs = sub->cs;

  if (!TEST_CHECK(cs_register_variables(cs, Vars, 0)))
    return;
  cs->init_complete = true;

  notify_observer_add(NeoMutt->notify, NT_CONFIG, log_observer, 0);

  set_list(cs);

  struct Buffer *err = mutt_buffer_pool_get();
  TEST_CHECK(test_initial_values(sub, err));
  TEST_CHECK(test_string_set(sub, err));
  TEST_CHECK(test_string_get(sub, err));
  TEST_CHECK(test_native_set(sub, err));
  TEST_CHECK(test_native_get(sub, err));
  TEST_CHECK(test_reset(sub, err));
  TEST_CHECK(test_validator(sub, err));
  TEST_CHECK(test_inherit(cs, err));
  mutt_buffer_pool_release(&err);

  test_neomutt_destroy(&NeoMutt);
  log_line(__func__);
}
