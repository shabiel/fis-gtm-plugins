#include "gtmxc_types.h"

#define OPENSSL_YDB_WRAPPER_OK 0
#define OPENSSL_YDB_WRAPPER_E_WRONGARGS -1
#define OPENSSL_YDB_WRAPPER_E_DIGEST_NOT_SPECIFIED -2
#define OPENSSL_YDB_WRAPPER_E_DIGEST_NOT_FOUND -3
#define OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR -4
#define OPENSSL_YDB_WRAPPER_E_STRNCPY_ERROR -5
#define OPENSSL_YDB_WRAPPER_E_CIPHER_NOT_SPECIFIED -6
#define OPENSSL_YDB_WRAPPER_E_CIPHER_NOT_FOUND -7
#define OPENSSL_YDB_WRAPPER_E_KEYLEN_ERROR -8

gtm_status_t openssl_md(int argc, gtm_char_t *input, gtm_char_t *digest, gtm_char_t *output);
gtm_status_t openssl_md_init(int argc, gtm_char_t *digest);
gtm_status_t openssl_md_add(int argc, gtm_char_t *input);
gtm_status_t openssl_md_finish(int argc, gtm_char_t *output);
gtm_status_t openssl_base64e(int argc, gtm_char_t* input, gtm_char_t* output);
gtm_status_t openssl_base64d(int argc, gtm_char_t* input, gtm_char_t* output);
gtm_status_t openssl_crypt(int argc, gtm_string_t *input, gtm_char_t *cipher_name, gtm_char_t *key, gtm_char_t *IV, gtm_long_t mode, gtm_string_t *output);
