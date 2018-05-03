#include "gtmxc_types.h"

#define OPENSSL_YDB_WRAPPER_OK 0
#define OPENSSL_YDB_WRAPPER_E_WRONGARGS -1
#define OPENSSL_YDB_WRAPPER_E_DIGEST_NOT_SPECIFIED -2
#define OPENSSL_YDB_WRAPPER_E_DIGEST_NOT_FOUND -3
#define OPENSSL_YDB_WRAPPER_E_OPENSSL_ERROR -4
#define OPENSSL_YDB_WRAPPER_E_STRNCPY_ERROR -5

gtm_status_t openssl_md(int argc, gtm_char_t *input, gtm_char_t *digest, gtm_char_t *output);
gtm_status_t openssl_md_init(int argc, gtm_char_t *digest);
gtm_status_t openssl_md_add(int argc, gtm_char_t *input);
gtm_status_t openssl_md_finish(int argc, gtm_char_t *output);
