/**
 * @file
 * Parse a help document's liquid and markdown
 *
 * @authors
 * Copyright (C) 2020 Louis Brauer <louis77@member.fsf.org>
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

#include <stdbool.h>
#include "private.h"
#include "mutt/buffer.h"
#include "mutt/array.h"


/**
 * @page help_parse_liquid 
 *
 * Replace liquid tags by keyword from a HeaderArray
 * 
 * @param sbuf source buffer containing the text to be parsed
 * @param dbuf destination buffer 
 * @param ha a header array built while parsing the help doc yaml header
 * @retval False if parsing didn't succeed
 */


bool help_parse_liquid(const struct Buffer *sbuf, struct Buffer *dbuf, const struct HeaderArray *ha)
{
  if (!sbuf || !sbuf->data || !sbuf->dsize || !dbuf || !ha) 
    return false;

  // Compile Regex

  regex_t rg;
  char errmsg[256];
  char pattern[] = "\\{\\{ page\\.(.[a-z]*) \\}\\}";
  int rg_comp = regcomp(&rg, pattern, REG_EXTENDED);
  if (rg_comp)
  {
    regerror(rg_comp, &rg, errmsg, 256);
    mutt_debug(LL_DEBUG1, "REGEX error message: %s\n", errmsg);
    goto fail;
  }

  // Find & replaces tags

  regmatch_t rg_result[2];
  char * cursor = sbuf->data;
  for(;;) 
  {
    int rg_res = regexec(&rg, cursor, 2, rg_result, 0);
    if (rg_res)
    {
      regerror(rg_res, &rg, errmsg, 256);
      mutt_debug(LL_DEBUG1, "REGEX error message: %s\n", errmsg);
      break;
    }
    else 
    {
        char keyword[64] = "";
        char fullword[128] = "";

        size_t fullword_s = rg_result[0].rm_eo - rg_result[0].rm_so;
        strncpy(fullword, (char *)(cursor + rg_result[0].rm_so), fullword_s);

        size_t keyword_s = rg_result[1].rm_eo - rg_result[1].rm_so;
        strncpy(keyword, (char *)(cursor + rg_result[1].rm_so), keyword_s);

        // use mutt_strn_dup()?
        // use mutt_strn_cpy()?

        // Lookup keyword and replace fullword with value if found
        
        mutt_debug(LL_DEBUG1, "REGEX match: %s (%d - %d)\n", fullword, rg_result[0].rm_so, rg_result[0].rm_eo);
        mutt_debug(LL_DEBUG1, "REGEX sub1: %s (%d - %d)\n", keyword, rg_result[1].rm_so, rg_result[1].rm_eo);

        // Copy everything before fullword
        mutt_buffer_addstr_n(dbuf, cursor, rg_result[0].rm_so);

        mutt_debug(LL_DEBUG1, "Prefix copied");

        struct HelpFileHeader *header = help_file_hdr_find(keyword, ha);
        if (header)
        {
          mutt_debug(LL_DEBUG1, "Found header in HA, value: %s\n", header->val);
          mutt_buffer_addstr(dbuf, header->val);
        }
        else 
        {
          mutt_debug(LL_DEBUG1, "Header not found, will copy the original fullword\n");
          mutt_buffer_addstr(dbuf, fullword);          
        }

        cursor += (rg_result[0].rm_eo);
    }
  }

  // Copy rest of buffer 
  mutt_buffer_addstr(dbuf, cursor);

  regfree(&rg);
  return true;

fail:
  return false;
}


