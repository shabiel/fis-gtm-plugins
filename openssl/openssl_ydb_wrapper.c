#include <string.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include "openssl_ydb_wrapper.h"


/* Author: Sam Habiel, Pharm.D. (c) 2018
 * Licensed under Apache 2.0 */

/* Many thanks to various users on stack overflow, esp to 
 * https://stackoverflow.com/questions/7521007/sha1-hash-binary-20-to-string-41
 * and the man page for EVP_*.
 */

/* To compile: */
/* cc -I $gtm_dist -ansi -O2 -Wall -fPIC -shared openssl_ydb_wrapper.c -o openssl_ydb_wrapper.so -lcrypto */

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
  return (gtm_status_t)OPENSSL_YDB_WRAPPER_OK;
}
gtm_status_t openssl_base64d(int argc, gtm_char_t* input, gtm_char_t* output)
{
  if (argc != 2) return (gtm_status_t)-1;
  int bytesDecoded = EVP_DecodeBlock((unsigned char *)output, (const unsigned char *)input, strlen(input));
  if (bytesDecoded == -1) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;
  return (gtm_status_t)OPENSSL_YDB_WRAPPER_OK;
}

void cleanup(EVP_CIPHER_CTX *ctx)
{
  ERR_free_strings();
  EVP_CIPHER_CTX_free(ctx);
}

gtm_status_t openssl_crypt(int argc, gtm_string_t *input, gtm_char_t *cipher_name, gtm_char_t *key, gtm_char_t *IV, gtm_long_t mode, gtm_string_t *output)
{
  /* We have to use gtm_string_t for input and output b/c encrypted data can contained embedded zeros */

  int ciphertext_len; /* temp variable for incrementing into output */

  /* Sanity checks */
  
  if (argc != 6)    return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_WRONGARGS;
  if (!cipher_name) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_CIPHER_NOT_SPECIFIED;

  /* Load error strings for printing to stderr */
  ERR_load_crypto_strings();

  /* Get cipher pointer */
  OpenSSL_add_all_algorithms();
  const EVP_CIPHER *cipher;
  cipher = EVP_get_cipherbyname((char *)cipher_name);
  if (!cipher) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_CIPHER_NOT_FOUND;
  
  /* Create context */
  EVP_CIPHER_CTX *ctx;
  ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;

  /* Init1 to check key lengths on the context */
  EVP_CipherInit_ex(ctx, cipher, NULL, NULL, NULL, (int)mode);
  if (strlen(key)  != EVP_CIPHER_CTX_key_length(ctx)) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_KEYLEN_ERROR;
  if (strlen(IV)   != EVP_CIPHER_CTX_iv_length(ctx))  return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_KEYLEN_ERROR;

  /* Set key and IV into the context */
  if (!EVP_CipherInit_ex(ctx, NULL, NULL, (const unsigned char *)key, (const unsigned char *)IV, (int)mode))
  {
    cleanup(ctx);
    return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;
  }
  
  /* Encrypt/Decrypt */
  if (!EVP_CipherUpdate(ctx, (unsigned char *)output->address, &ciphertext_len, (const unsigned char *)input->address, input->length))
  {
    cleanup(ctx);
    return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;
  }
  
  /* Set current length */
  output->length = ciphertext_len;
  /* fprintf(stderr,"cipher len %d\n", ciphertext_len); */
  
  /* Finish */
  if (!EVP_CipherFinal_ex(ctx, (unsigned char *)output->address + output->length, &ciphertext_len))
  {
    ERR_print_errors_fp(stderr);
    cleanup(ctx);
    return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;
  }
  
  /* fprintf(stderr,"cipher len %d\n", ciphertext_len); */
  /* increment length of output */
  output->length += ciphertext_len;

  /* Clean and return */
  cleanup(ctx);
  return (gtm_status_t)OPENSSL_YDB_WRAPPER_OK;
}

/* This is just for testing */
/* cc -I $gtm_dist -ansi -g -O0 -Wall -fPIC openssl_ydb_wrapper.c -o openssl_ydb_wrapper -lcrypto */
int main()
{
  /* The follow below is equivalent to:
   * echo -n "aaa" | openssl enc -e -aes-128-cbc -K 30313233343536373839616263646546 -iv 31323334353637383837363534333231 */
  size_t output_size = 32768;
  gtm_string_t input;
  input.length  = strlen("aaa") + 1;
  input.address = "aaa";
  char cipher[] = "AES-128-CBC";
  char key[] = "0123456789abcdef";
  char iv[] = "1234567887654321";
  gtm_string_t output;
  output.address = (char *)malloc(output_size);
  output.length = 0;
  unsigned char result = openssl_crypt(6, &input, cipher, key, iv, 1, &output);

  if (result) {
    printf("error %d", result);
    return result;
  }
  BIO_dump_fp(stdout, (const char *)output.address, output.length);

  gtm_string_t output2;
  output2.address = (char *)malloc(output_size);
  output2.length = 0;
  
  result = openssl_crypt(6, &output, cipher, key, iv, 0, &output2);
  if (result) {
    printf("error %d", result);
    return result;
  }
  printf("%s\n", output2.address);

  free(output.address);
  free(output2.address);

  /* Try another example that currently fails */
  char *str = "Alice and Bob had Sex!";
  input.length  = strlen(str) + 1;
  input.address = str;
  /* 
  input.length  = strlen("aaa");
  input.address = "aaa";
  */
  char cipher2[] = "AES-256-CBC";
  char key2[] = "0123456789abcdeF0123456789abcdeF";
  char iv2[] = "1234567887654321";
  output.address = (char *)malloc(output_size);
  output.length = 0;
  result = openssl_crypt(6, &input, cipher2, key2, iv2, 1, &output);

  if (result) {
    printf("error %d", result);
    return result;
  }
  BIO_dump_fp(stdout, (const char *)output.address, output.length);

  output2.address = (char *)malloc(output_size);
  output2.length = 0;
  
  result = openssl_crypt(6, &output, cipher2, key2, iv2, 0, &output2);
  if (result) {
    printf("error %d", result);
    return result;
  }
  printf("%s\n", output2.address);

  free(output.address);
  free(output2.address);
}
