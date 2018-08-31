#include <string.h>
#include <openssl/evp.h>
#include "openssl_ydb_wrapper.h"

/* Author: Sam Habiel, Pharm.D. (c) 2018
 * Licensed under Apache 2.0 */

/* Many thanks to various users on stack overflow, esp to 
 * https://stackoverflow.com/questions/7521007/sha1-hash-binary-20-to-string-41
 * and the man page for EVP_*.
 */

/* To compile: */
/* cc -I $gtm_dist -ansi -O2 -Wall -fPIC -shared openssl_ydb_wrapper.c -o openssl_ydb_wrapper.so -lcrypto */

/* Error codes:
 * -1: Wrong number of arguments
 * -4: Digest type is not specified
 * -5: Digest type is not found
 * -6: openssl error
 * -7: strncpy error */


/* call with do &openssl.md(input,"sha1",.zzz)
 *      or   set status=$&openssl.md(input,"sha1,",.zzz)
 *
 * Inputs:
 * input: string to hash
 * digest: sha1/sha512/md5 etc.
 * output: pass by reference */
gtm_status_t openssl_md(int argc, gtm_char_t *input, gtm_char_t *digest, gtm_char_t *output)
{
  if (argc != 3) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_WRONGARGS;
  if (!digest) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_DIGEST_NOT_SPECIFIED;

  EVP_MD_CTX *mdctx;
  const EVP_MD *md;
  unsigned char md_value[EVP_MAX_MD_SIZE];
  unsigned int md_len;

  OpenSSL_add_all_digests();

  md = EVP_get_digestbyname((char *)digest);
  
  if (!md) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_DIGEST_NOT_FOUND;

  mdctx = EVP_MD_CTX_create();
  if (!mdctx) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;

  if (1 != EVP_DigestInit_ex(mdctx, md, NULL)) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;
  if (1 != EVP_DigestUpdate(mdctx, (unsigned char *)input, strlen((char *)input))) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;
  if (1 != EVP_DigestFinal_ex(mdctx, md_value, &md_len)) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;

  EVP_MD_CTX_destroy(mdctx);

  int hexOutputSize = md_len*2+1;
  char hexOutput[hexOutputSize];
  char hexAlphabet[] = "0123456789ABCDEF";

  int i;
  for (i = 0; i < md_len; i++) {
    hexOutput[2*i]     = hexAlphabet[md_value[i] / 16];
    hexOutput[2*i + 1] = hexAlphabet[md_value[i] % 16];
  }
  hexOutput[hexOutputSize - 1] = 0;
  
  if (NULL == strncpy(output, hexOutput, hexOutputSize)) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_STRNCPY_ERROR;

  /* Call this once before exit. */
  EVP_cleanup();

  return (gtm_status_t)OPENSSL_YDB_WRAPPER_OK;
}

EVP_MD_CTX *mdctx;

/* Inputs:
 * digest: digest type (sha1/sha512/etc)
 * No output
 *
 * call with do &openssl.init("sha1")
 */
gtm_status_t openssl_md_init(int argc, gtm_char_t *digest) 
{
  if (argc != 1) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_WRONGARGS;
  if (!digest) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_DIGEST_NOT_SPECIFIED;

  OpenSSL_add_all_digests();

  const EVP_MD *md;
  md = EVP_get_digestbyname((char *)digest);
  
  if(!md) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_DIGEST_NOT_FOUND;

  mdctx = EVP_MD_CTX_create();
  if (!mdctx) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;

  if (1 != EVP_DigestInit_ex(mdctx, md, NULL)) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;
  return (gtm_status_t)OPENSSL_YDB_WRAPPER_OK;
}

/* Inputs:
 * input: string to hash
 * No output
 *
 * call with do &openssl.add(input)
 */
gtm_status_t openssl_md_add(int argc, gtm_char_t *input) 
{
  if (argc != 1) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_WRONGARGS;
  if (1 != EVP_DigestUpdate(mdctx, (unsigned char *)input, strlen((char *)input))) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;
  return (gtm_status_t)OPENSSL_YDB_WRAPPER_OK;
}

/* Output: by reference
 * No inputs
 *
 * call with do &openssl.finish(.zzz)
 */
gtm_status_t openssl_md_finish(int argc, gtm_char_t *output)
{
  if (argc != 1) return (gtm_status_t)-1;
  unsigned char md_value[EVP_MAX_MD_SIZE];
  unsigned int md_len;

  if (1 != EVP_DigestFinal_ex(mdctx, md_value, &md_len)) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;
  EVP_MD_CTX_destroy(mdctx);

  int hexOutputSize = md_len*2+1;
  char hexOutput[hexOutputSize];
  char hexAlphabet[] = "0123456789ABCDEF";
  int i;
  for (i = 0; i < md_len; i++) {
    hexOutput[2*i]     = hexAlphabet[md_value[i] / 16];
    hexOutput[2*i + 1] = hexAlphabet[md_value[i] % 16];
  }
  hexOutput[hexOutputSize - 1] = 0;
  
  if (NULL == strncpy(output, hexOutput, hexOutputSize)) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_STRNCPY_ERROR;

  EVP_cleanup();

  return (gtm_status_t)OPENSSL_YDB_WRAPPER_OK;
}

/* Base 64 Encode/Decode
 * Output: by reference (up to 1M bytes)
 * Input: string to base64 encode
 *
 * call with do &openssl.base64e(input,.zzz)
 *           &  &openssl.base64d(input,.zzz)
 */
gtm_status_t openssl_base64e(int argc, gtm_char_t* input, gtm_char_t* output)
{
  if (argc != 2) return (gtm_status_t)-1;
  int bytesEncoded = EVP_EncodeBlock((unsigned char *)output, (const unsigned char *)input, strlen(input));
  if (bytesEncoded < 1) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;
  output[bytesEncoded+1] = 0; /* zero terminate string */
  return (gtm_status_t)OPENSSL_YDB_WRAPPER_OK;
}

gtm_status_t openssl_base64d(int argc, gtm_char_t* input, gtm_char_t* output)
{
  if (argc != 2) return (gtm_status_t)-1;
  int bytesDecoded = EVP_DecodeBlock((unsigned char *)output, (const unsigned char *)input, strlen(input));
  if (bytesDecoded == -1) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;
  output[bytesDecoded+1] = 0; /* zero terminate string */
  return (gtm_status_t)OPENSSL_YDB_WRAPPER_OK;
}

/* This is just for testing */
/* cc -I $gtm_dist -ansi -g -O0 -Wall -fPIC openssl_ydb_wrapper.c -o openssl_ydb_wrapper -lcrypto */
int main()
{
  char input[4] = "aaa";
  char output[32555];
  openssl_base64e(2, input, output);
  printf("%s", output);
}
