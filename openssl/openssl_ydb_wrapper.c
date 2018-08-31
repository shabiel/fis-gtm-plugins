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
  return (gtm_status_t)OPENSSL_YDB_WRAPPER_OK;
}
gtm_status_t openssl_base64d(int argc, gtm_char_t* input, gtm_char_t* output)
{
  if (argc != 2) return (gtm_status_t)-1;
  int bytesDecoded = EVP_DecodeBlock((unsigned char *)output, (const unsigned char *)input, strlen(input));
  if (bytesDecoded == -1) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;
  return (gtm_status_t)OPENSSL_YDB_WRAPPER_OK;
}

gtm_status_t openssl_crypt(int argc, gtm_string_t *input, gtm_char_t *cipher_name, gtm_char_t *key, gtm_char_t *IV, gtm_long_t mode, gtm_string_t *output)
{
	/* We have to use gtm_string_t for input and output b/c encrypted data can contained embedded zeros */
  /* Sanity checks */
  if (argc != 6)    return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_WRONGARGS;
  if (!cipher_name) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_CIPHER_NOT_SPECIFIED;

  /* Get cipher pointer */
  OpenSSL_add_all_algorithms();
  const EVP_CIPHER *cipher;
  cipher = EVP_get_cipherbyname((char *)cipher_name);
  if (!cipher) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_CIPHER_NOT_FOUND;
  
  /* Create context */
  EVP_CIPHER_CTX *ctx;
  ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;

  /* Encrypt */
  if (1 != EVP_CipherInit_ex(ctx, cipher, NULL, (const unsigned char *)key, (const unsigned char *)IV, (int)mode))
  {
    EVP_CIPHER_CTX_free(ctx);
    return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;
  }
  
  if (1 != EVP_CipherUpdate(ctx, (unsigned char *)output->address, (int *)&output->length, (const unsigned char *)input->address, input->length))
  {
    EVP_CIPHER_CTX_free(ctx);
    return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;
  }
  
  if (1 != EVP_CipherFinal_ex(ctx, (unsigned char *)output->address, (int *)&output->length))
  {
		ERR_print_errors_fp(stderr);
    EVP_CIPHER_CTX_free(ctx);
    return (gtm_status_t)OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR;
  }
	
  EVP_CIPHER_CTX_free(ctx);
  return (gtm_status_t)OPENSSL_YDB_WRAPPER_OK;
}

/* This is just for testing */
/* cc -I $gtm_dist -ansi -g -O0 -Wall -fPIC openssl_ydb_wrapper.c -o openssl_ydb_wrapper -lcrypto */
int main()
{
	ERR_load_crypto_strings();
	/* The follow below is equivalent to:
	 * echo -n "aaa" | openssl enc -e -aes-128-cbc -K 30313233343536373839616263646546 -iv 31323334353637383837363534333231 */
	size_t output_size = 32768;
	gtm_string_t input;
	input.length  = strlen("aaa");
	input.address = "aaa";
  char cipher[] = "AES-128-CBC";
  char key[] = "0123456789abcdeF";
  char iv[] = "1234567887654321";
	gtm_string_t output;
	output.address = (char *)malloc(output_size);
	output.length = 0;
  unsigned char result = openssl_crypt(6, &input, cipher, key, iv, 1, &output);

  if (result) {
    printf("error %d", result);
    return result;
  }
  printf("%s\n", output.address);

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
	input.length  = strlen("Alice and Bob had Sex!");
	input.address = "Alice and Bob had Sex!";
  char cipher2[] = "AES-256-CBC";
  char key2[] = "0123456789abcdeF";
  char iv2[] = "1234567887654321";
	output.address = (char *)malloc(output_size);
	output.length = 0;
  result = openssl_crypt(6, &input, cipher2, key2, iv2, 1, &output);

  if (result) {
    printf("error %d", result);
    return result;
  }
  printf("%s\n", output.address);

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
