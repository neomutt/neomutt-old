/**
 * @file
 * Wrapper around calls to external PGP program
 *
 * @authors
 * Copyright (C) 1997-2003 Thomas Roessler <roessler@does-not-exist.org>
 * Copyright (C) 2019 Pietro Cerutti <gahr@gahr.ch>
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
 * @page crypt_pgpinvoke Wrapper around calls to external PGP program
 *
 * This file contains the new pgp invocation code.
 *
 * @note This is almost entirely format based.
 */

#include "config.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "mutt/lib.h"
#include "address/lib.h"
#include "config/lib.h"
#include "core/lib.h"
#include "gui/lib.h"
#include "lib.h"
#include "expando/lib.h"
#include "format_flags.h"
#include "mutt_logging.h"
#include "muttlib.h"
#include "pgpinvoke.h"
#include "pgpkey.h"
#include "protos.h"
#ifdef CRYPT_BACKEND_CLASSIC_PGP
#include "pgp.h"
#endif

/**
 * mutt_pgp_command - Prepare a PGP Command
 * @param buf    Buffer for the result
 * @param buflen Length of buffer
 * @param cctx   Data to pass to the formatter
 * @param fmt    printf-like formatting string
 *
 * @sa pgp_command_format_str()
 */
static void mutt_pgp_command(char *buf, size_t buflen, struct PgpCommandContext *cctx,
                             const struct ExpandoRecord *record)
{
  mutt_expando_format_2gmb(buf, buflen, buflen, record, (intptr_t) cctx, MUTT_FORMAT_NO_FLAGS);
  mutt_debug(LL_DEBUG2, "%s\n", buf);
}

/**
 * pgp_invoke - Run a PGP command
 * @param[out] fp_pgp_in       stdin  for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_out      stdout for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_err      stderr for the command, or NULL (OPTIONAL)
 * @param[in]  fd_pgp_in       stdin  for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_out      stdout for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_err      stderr for the command, or -1 (OPTIONAL)
 * @param[in]  need_passphrase Is a passphrase needed?
 * @param[in]  fname           Filename to pass to the command
 * @param[in]  sig_fname       Signature filename to pass to the command
 * @param[in]  ids             List of IDs/fingerprints, space separated
 * @param[in]  format          printf-like format string
 * @retval num PID of the created process
 * @retval -1  Error creating pipes or forking
 *
 * @note `fp_pgp_in` has priority over `fd_pgp_in`.
 *       Likewise `fp_pgp_out` and `fp_pgp_err`.
 */
static pid_t pgp_invoke(FILE **fp_pgp_in, FILE **fp_pgp_out, FILE **fp_pgp_err,
                        int fd_pgp_in, int fd_pgp_out, int fd_pgp_err,
                        bool need_passphrase, const char *fname, const char *sig_fname,
                        const char *ids, const struct ExpandoRecord *r)
{
  struct PgpCommandContext cctx = { 0 };
  char cmd[STR_COMMAND] = { 0 };

  if (!r)
  {
    return (pid_t) -1;
  }

  cctx.need_passphrase = need_passphrase;
  cctx.fname = fname;
  cctx.sig_fname = sig_fname;
  const char *const c_pgp_sign_as = cs_subset_string(NeoMutt->sub, "pgp_sign_as");
  const char *const c_pgp_default_key = cs_subset_string(NeoMutt->sub, "pgp_default_key");
  if (c_pgp_sign_as)
    cctx.signas = c_pgp_sign_as;
  else
    cctx.signas = c_pgp_default_key;
  cctx.ids = ids;

  mutt_pgp_command(cmd, sizeof(cmd), &cctx, r);

  return filter_create_fd(cmd, fp_pgp_in, fp_pgp_out, fp_pgp_err, fd_pgp_in,
                          fd_pgp_out, fd_pgp_err);
}

/*
 * The exported interface.
 *
 * This is historic and may be removed at some point.
 */

/**
 * pgp_invoke_decode - Use PGP to decode a message
 * @param[out] fp_pgp_in       stdin  for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_out      stdout for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_err      stderr for the command, or NULL (OPTIONAL)
 * @param[in]  fd_pgp_in       stdin  for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_out      stdout for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_err      stderr for the command, or -1 (OPTIONAL)
 * @param[in]  fname           Filename to pass to the command
 * @param[in]  need_passphrase Is a passphrase needed?
 * @retval num PID of the created process
 * @retval -1  Error creating pipes or forking
 *
 * @note `fp_pgp_in` has priority over `fd_pgp_in`.
 *       Likewise `fp_pgp_out` and `fp_pgp_err`.
 */
pid_t pgp_invoke_decode(FILE **fp_pgp_in, FILE **fp_pgp_out, FILE **fp_pgp_err,
                        int fd_pgp_in, int fd_pgp_out, int fd_pgp_err,
                        const char *fname, bool need_passphrase)
{
  const struct ExpandoRecord *c_pgp_decode_command = cs_subset_expando(NeoMutt->sub, "pgp_decode_command");
  return pgp_invoke(fp_pgp_in, fp_pgp_out, fp_pgp_err, fd_pgp_in, fd_pgp_out, fd_pgp_err,
                    need_passphrase, fname, NULL, NULL, c_pgp_decode_command);
}

/**
 * pgp_invoke_verify - Use PGP to verify a message
 * @param[out] fp_pgp_in  stdin  for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_out stdout for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_err stderr for the command, or NULL (OPTIONAL)
 * @param[in]  fd_pgp_in  stdin  for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_out stdout for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_err stderr for the command, or -1 (OPTIONAL)
 * @param[in]  fname      Filename to pass to the command
 * @param[in]  sig_fname  Signature filename to pass to the command
 * @retval num PID of the created process
 * @retval -1  Error creating pipes or forking
 *
 * @note `fp_pgp_in` has priority over `fd_pgp_in`.
 *       Likewise `fp_pgp_out` and `fp_pgp_err`.
 */
pid_t pgp_invoke_verify(FILE **fp_pgp_in, FILE **fp_pgp_out, FILE **fp_pgp_err,
                        int fd_pgp_in, int fd_pgp_out, int fd_pgp_err,
                        const char *fname, const char *sig_fname)
{
  const struct ExpandoRecord *c_pgp_verify_command = cs_subset_expando(NeoMutt->sub, "pgp_verify_command");
  return pgp_invoke(fp_pgp_in, fp_pgp_out, fp_pgp_err, fd_pgp_in, fd_pgp_out,
                    fd_pgp_err, false, fname, sig_fname, NULL, c_pgp_verify_command);
}

/**
 * pgp_invoke_decrypt - Use PGP to decrypt a file
 * @param[out] fp_pgp_in  stdin  for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_out stdout for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_err stderr for the command, or NULL (OPTIONAL)
 * @param[in]  fd_pgp_in  stdin  for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_out stdout for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_err stderr for the command, or -1 (OPTIONAL)
 * @param[in]  fname      Filename to pass to the command
 * @retval num PID of the created process
 * @retval -1  Error creating pipes or forking
 *
 * @note `fp_pgp_in` has priority over `fd_pgp_in`.
 *       Likewise `fp_pgp_out` and `fp_pgp_err`.
 */
pid_t pgp_invoke_decrypt(FILE **fp_pgp_in, FILE **fp_pgp_out, FILE **fp_pgp_err,
                         int fd_pgp_in, int fd_pgp_out, int fd_pgp_err, const char *fname)
{
  const struct ExpandoRecord *c_pgp_decrypt_command = cs_subset_expando(NeoMutt->sub, "pgp_decrypt_command");
  return pgp_invoke(fp_pgp_in, fp_pgp_out, fp_pgp_err, fd_pgp_in, fd_pgp_out,
                    fd_pgp_err, true, fname, NULL, NULL, c_pgp_decrypt_command);
}

/**
 * pgp_invoke_sign - Use PGP to sign a file
 * @param[out] fp_pgp_in  stdin  for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_out stdout for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_err stderr for the command, or NULL (OPTIONAL)
 * @param[in]  fd_pgp_in  stdin  for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_out stdout for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_err stderr for the command, or -1 (OPTIONAL)
 * @param[in]  fname      Filename to pass to the command
 * @retval num PID of the created process
 * @retval -1  Error creating pipes or forking
 *
 * @note `fp_pgp_in` has priority over `fd_pgp_in`.
 *       Likewise `fp_pgp_out` and `fp_pgp_err`.
 */
pid_t pgp_invoke_sign(FILE **fp_pgp_in, FILE **fp_pgp_out, FILE **fp_pgp_err,
                      int fd_pgp_in, int fd_pgp_out, int fd_pgp_err, const char *fname)
{
  const struct ExpandoRecord *c_pgp_sign_command = cs_subset_expando(NeoMutt->sub, "pgp_sign_command");
  return pgp_invoke(fp_pgp_in, fp_pgp_out, fp_pgp_err, fd_pgp_in, fd_pgp_out,
                    fd_pgp_err, true, fname, NULL, NULL, c_pgp_sign_command);
}

/**
 * pgp_invoke_encrypt - Use PGP to encrypt a file
 * @param[out] fp_pgp_in  stdin  for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_out stdout for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_err stderr for the command, or NULL (OPTIONAL)
 * @param[in]  fd_pgp_in  stdin  for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_out stdout for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_err stderr for the command, or -1 (OPTIONAL)
 * @param[in]  fname      Filename to pass to the command
 * @param[in]  uids       List of IDs/fingerprints, space separated
 * @param[in]  sign       If true, also sign the file
 * @retval num PID of the created process
 * @retval -1  Error creating pipes or forking
 *
 * @note `fp_pgp_in` has priority over `fd_pgp_in`.
 *       Likewise `fp_pgp_out` and `fp_pgp_err`.
 */
pid_t pgp_invoke_encrypt(FILE **fp_pgp_in, FILE **fp_pgp_out, FILE **fp_pgp_err,
                         int fd_pgp_in, int fd_pgp_out, int fd_pgp_err,
                         const char *fname, const char *uids, bool sign)
{
  if (sign)
  {
    const struct ExpandoRecord *c_pgp_encrypt_sign_command =
        cs_subset_expando(NeoMutt->sub, "pgp_encrypt_sign_command");
    return pgp_invoke(fp_pgp_in, fp_pgp_out, fp_pgp_err, fd_pgp_in, fd_pgp_out,
                      fd_pgp_err, true, fname, NULL, uids, c_pgp_encrypt_sign_command);
  }
  else
  {
    const struct ExpandoRecord *c_pgp_encrypt_only_command =
        cs_subset_expando(NeoMutt->sub, "pgp_encrypt_only_command");
    return pgp_invoke(fp_pgp_in, fp_pgp_out, fp_pgp_err, fd_pgp_in, fd_pgp_out,
                      fd_pgp_err, false, fname, NULL, uids, c_pgp_encrypt_only_command);
  }
}

/**
 * pgp_invoke_traditional - Use PGP to create in inline-signed message
 * @param[out] fp_pgp_in  stdin  for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_out stdout for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_err stderr for the command, or NULL (OPTIONAL)
 * @param[in]  fd_pgp_in  stdin  for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_out stdout for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_err stderr for the command, or -1 (OPTIONAL)
 * @param[in]  fname      Filename to pass to the command
 * @param[in]  uids       List of IDs/fingerprints, space separated
 * @param[in]  flags      Flags, see #SecurityFlags
 * @retval num PID of the created process
 * @retval -1  Error creating pipes or forking
 *
 * @note `fp_pgp_in` has priority over `fd_pgp_in`.
 *       Likewise `fp_pgp_out` and `fp_pgp_err`.
 */
pid_t pgp_invoke_traditional(FILE **fp_pgp_in, FILE **fp_pgp_out, FILE **fp_pgp_err,
                             int fd_pgp_in, int fd_pgp_out, int fd_pgp_err,
                             const char *fname, const char *uids, SecurityFlags flags)
{
  if (flags & SEC_ENCRYPT)
  {
    const struct ExpandoRecord *c_pgp_encrypt_only_command =
        cs_subset_expando(NeoMutt->sub, "pgp_encrypt_only_command");
    const struct ExpandoRecord *c_pgp_encrypt_sign_command =
        cs_subset_expando(NeoMutt->sub, "pgp_encrypt_sign_command");
    return pgp_invoke(fp_pgp_in, fp_pgp_out, fp_pgp_err, fd_pgp_in, fd_pgp_out,
                      fd_pgp_err, (flags & SEC_SIGN), fname, NULL, uids,
                      (flags & SEC_SIGN) ? c_pgp_encrypt_sign_command : c_pgp_encrypt_only_command);
  }
  else
  {
    const struct ExpandoRecord *c_pgp_clear_sign_command =
        cs_subset_expando(NeoMutt->sub, "pgp_clear_sign_command");
    return pgp_invoke(fp_pgp_in, fp_pgp_out, fp_pgp_err, fd_pgp_in, fd_pgp_out,
                      fd_pgp_err, true, fname, NULL, NULL, c_pgp_clear_sign_command);
  }
}

/**
 * pgp_class_invoke_import - Implements CryptModuleSpecs::pgp_invoke_import() - @ingroup crypto_pgp_invoke_import
 */
void pgp_class_invoke_import(const char *fname)
{
  char cmd[STR_COMMAND] = { 0 };
  struct PgpCommandContext cctx = { 0 };

  struct Buffer *buf_fname = buf_pool_get();

  buf_quote_filename(buf_fname, fname, true);
  cctx.fname = buf_string(buf_fname);
  const char *const c_pgp_sign_as = cs_subset_string(NeoMutt->sub, "pgp_sign_as");
  const char *const c_pgp_default_key = cs_subset_string(NeoMutt->sub, "pgp_default_key");
  if (c_pgp_sign_as)
    cctx.signas = c_pgp_sign_as;
  else
    cctx.signas = c_pgp_default_key;

  const struct ExpandoRecord *c_pgp_import_command = cs_subset_expando(NeoMutt->sub, "pgp_import_command");
  mutt_pgp_command(cmd, sizeof(cmd), &cctx, c_pgp_import_command);
  if (mutt_system(cmd) != 0)
    mutt_debug(LL_DEBUG1, "Error running \"%s\"\n", cmd);

  buf_pool_release(&buf_fname);
}

/**
 * pgp_class_invoke_getkeys - Implements CryptModuleSpecs::pgp_invoke_getkeys() - @ingroup crypto_pgp_invoke_getkeys
 */
void pgp_class_invoke_getkeys(struct Address *addr)
{
  char cmd[STR_COMMAND] = { 0 };

  struct Buffer *personal = NULL;

  struct PgpCommandContext cctx = { 0 };

  const struct ExpandoRecord *c_pgp_get_keys_command = cs_subset_expando(NeoMutt->sub, "pgp_get_keys_command");
  if (!c_pgp_get_keys_command)
    return;

  struct Buffer *buf = buf_pool_get();
  personal = addr->personal;
  addr->personal = NULL;

  struct Buffer *tmp = buf_pool_get();
  mutt_addr_to_local(addr);
  mutt_addr_write(tmp, addr, false);
  buf_quote_filename(buf, buf_string(tmp), true);
  buf_pool_release(&tmp);

  addr->personal = personal;

  cctx.ids = buf_string(buf);

  mutt_pgp_command(cmd, sizeof(cmd), &cctx, c_pgp_get_keys_command);

  int fd_null = open("/dev/null", O_RDWR);

  if (!isendwin())
    mutt_message(_("Fetching PGP key..."));

  if (mutt_system(cmd) != 0)
    mutt_debug(LL_DEBUG1, "Error running \"%s\"\n", cmd);

  if (!isendwin())
    mutt_clear_error();

  if (fd_null >= 0)
    close(fd_null);

  buf_pool_release(&buf);
}

/**
 * pgp_invoke_export - Use PGP to export a key from the user's keyring
 * @param[out] fp_pgp_in  stdin  for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_out stdout for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_err stderr for the command, or NULL (OPTIONAL)
 * @param[in]  fd_pgp_in  stdin  for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_out stdout for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_err stderr for the command, or -1 (OPTIONAL)
 * @param[in]  uids       List of IDs/fingerprints, space separated
 * @retval num PID of the created process
 * @retval -1  Error creating pipes or forking
 *
 * @note `fp_pgp_in` has priority over `fd_pgp_in`.
 *       Likewise `fp_pgp_out` and `fp_pgp_err`.
 */
pid_t pgp_invoke_export(FILE **fp_pgp_in, FILE **fp_pgp_out, FILE **fp_pgp_err,
                        int fd_pgp_in, int fd_pgp_out, int fd_pgp_err, const char *uids)
{
  const struct ExpandoRecord *c_pgp_export_command = cs_subset_expando(NeoMutt->sub, "pgp_export_command");
  return pgp_invoke(fp_pgp_in, fp_pgp_out, fp_pgp_err, fd_pgp_in, fd_pgp_out,
                    fd_pgp_err, false, NULL, NULL, uids, c_pgp_export_command);
}

/**
 * pgp_invoke_verify_key - Use PGP to verify a key
 * @param[out] fp_pgp_in  stdin  for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_out stdout for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_err stderr for the command, or NULL (OPTIONAL)
 * @param[in]  fd_pgp_in  stdin  for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_out stdout for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_err stderr for the command, or -1 (OPTIONAL)
 * @param[in]  uids       List of IDs/fingerprints, space separated
 * @retval num PID of the created process
 * @retval -1  Error creating pipes or forking
 *
 * @note `fp_pgp_in` has priority over `fd_pgp_in`.
 *       Likewise `fp_pgp_out` and `fp_pgp_err`.
 */
pid_t pgp_invoke_verify_key(FILE **fp_pgp_in, FILE **fp_pgp_out, FILE **fp_pgp_err,
                            int fd_pgp_in, int fd_pgp_out, int fd_pgp_err, const char *uids)
{
  const struct ExpandoRecord *c_pgp_verify_key_command =
      cs_subset_expando(NeoMutt->sub, "pgp_verify_key_command");
  return pgp_invoke(fp_pgp_in, fp_pgp_out, fp_pgp_err, fd_pgp_in, fd_pgp_out,
                    fd_pgp_err, false, NULL, NULL, uids, c_pgp_verify_key_command);
}

/**
 * pgp_invoke_list_keys - Find matching PGP Keys
 * @param[out] fp_pgp_in  stdin  for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_out stdout for the command, or NULL (OPTIONAL)
 * @param[out] fp_pgp_err stderr for the command, or NULL (OPTIONAL)
 * @param[in]  fd_pgp_in  stdin  for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_out stdout for the command, or -1 (OPTIONAL)
 * @param[in]  fd_pgp_err stderr for the command, or -1 (OPTIONAL)
 * @param[in]  keyring    Keyring type, e.g. #PGP_SECRING
 * @param[in]  hints      Match keys to these strings
 * @retval num PID of the created process
 * @retval -1  Error creating pipes or forking
 *
 * @note `fp_pgp_in` has priority over `fd_pgp_in`.
 *       Likewise `fp_pgp_out` and `fp_pgp_err`.
 */
pid_t pgp_invoke_list_keys(FILE **fp_pgp_in, FILE **fp_pgp_out, FILE **fp_pgp_err,
                           int fd_pgp_in, int fd_pgp_out, int fd_pgp_err,
                           enum PgpRing keyring, struct ListHead *hints)
{
  struct Buffer *uids = buf_pool_get();
  struct Buffer *quoted = buf_pool_get();

  struct ListNode *np = NULL;
  STAILQ_FOREACH(np, hints, entries)
  {
    buf_quote_filename(quoted, (char *) np->data, true);
    buf_addstr(uids, buf_string(quoted));
    if (STAILQ_NEXT(np, entries))
      buf_addch(uids, ' ');
  }

  const struct ExpandoRecord *c_pgp_list_pubring_command =
      cs_subset_expando(NeoMutt->sub, "pgp_list_pubring_command");
  const struct ExpandoRecord *c_pgp_list_secring_command =
      cs_subset_expando(NeoMutt->sub, "pgp_list_secring_command");
  pid_t rc = pgp_invoke(fp_pgp_in, fp_pgp_out, fp_pgp_err, fd_pgp_in,
                        fd_pgp_out, fd_pgp_err, 0, NULL, NULL, buf_string(uids),
                        (keyring == PGP_SECRING) ? c_pgp_list_secring_command :
                                                   c_pgp_list_pubring_command);

  buf_pool_release(&uids);
  buf_pool_release(&quoted);
  return rc;
}
