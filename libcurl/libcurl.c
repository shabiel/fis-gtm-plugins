/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2018, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * Copyright (C) 2018,2023 Sam Habiel - Modifications to work on GT.M/YDB
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include "gtmxc_types.h"

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(ptr == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

static size_t header_callback(char *buffer, size_t size,
                              size_t nitems, void *return_headers)
{
  size_t realsize = size * nitems;
  struct MemoryStruct *mem = (struct MemoryStruct *)return_headers;
  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(ptr == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), buffer, realsize);
  mem->size += realsize;

  return nitems * size;
}

CURL *curl_handle;
struct curl_slist *hs;

gtm_status_t curl_init()
{
  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = curl_easy_init();

  return (gtm_status_t)0;
}

gtm_status_t curl_add_header(int argc, gtm_char_t *header)
{
  hs = curl_slist_append(hs, (char* )header);
  return (gtm_status_t)0;
}

gtm_status_t curl_cleanup()
{
  /* cleanup curl stuff */
  if (curl_handle != NULL)
  {
    curl_easy_cleanup(curl_handle);

    /* we're done with libcurl, so clean it up */
    curl_global_cleanup();

    curl_handle = NULL;
  }

  return (gtm_status_t)0;
}

/* Client TLS */
gtm_status_t curl_client_tls(int argc,
    gtm_char_t *certFile,              /* 1 */
    gtm_char_t *privateKeyFile,        /* 2 */
    gtm_char_t *privateKeyPassword,    /* 3 */
    gtm_char_t *CABundleFile)          /* 4 */
{
  /* first two arguments required - Certificate and Key */
  curl_easy_setopt(curl_handle, CURLOPT_SSLCERT, (char *)certFile);
  curl_easy_setopt(curl_handle, CURLOPT_SSLKEY,  (char *)privateKeyFile);
  curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1);

  /* Key password */
  if (argc >= 3 && strlen(privateKeyPassword))
  {
     curl_easy_setopt(curl_handle, CURLOPT_KEYPASSWD, privateKeyPassword);
  }

  /* Apply CABundleFile if requested */
  if (argc >= 4 && strlen(CABundleFile))
  {
    curl_easy_setopt(curl_handle, CURLOPT_CAINFO,  (char *)CABundleFile);
  }

  return (gtm_status_t)0;
}

/* Server self-signed certificate support */
gtm_status_t curl_server_ca(int argc, gtm_char_t *CABundleFile)
{
  curl_easy_setopt(curl_handle, CURLOPT_CAINFO, (char *)CABundleFile);
  curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1);
  return (gtm_status_t)0;
}

/* Call to support HTTP AUTH */
gtm_status_t curl_auth(int argc, gtm_char_t *auth_type, gtm_char_t *unpw)
{
  /* Right now, there is nothing else besides basic supported; so I am
   * not even checking the auth type parameter */
  curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
  curl_easy_setopt(curl_handle, CURLOPT_USERPWD, (char *)unpw);
  return (gtm_status_t)0;
}

/* Perform the curl operation */
/* Return:
 *   0 - ok,
 *   255 - output not compatible with plugin,
 *   -1 input arguments error */
gtm_status_t curl_do(int argc,
    gtm_long_t* http_status,      /* 1 */
    gtm_string_t *output,         /* 2 */
    gtm_char_t *method,           /* 3 */
    gtm_char_t *URL,              /* 4 */
    gtm_string_t *payload,        /* 5 */
    gtm_char_t *mime,             /* 6 */
    gtm_long_t timeout,           /* 7 */
    gtm_string_t *output_headers) /* 8 */
{
  /* CURL result code */
  CURLcode curl_result;

  /* Must have at least 4 arguments */
  if(argc < 4) return (gtm_status_t)-1;

  /* curl's body return */
  struct MemoryStruct return_body;
  return_body.memory = malloc(1);  /* will be grown as needed by the realloc above */
  return_body.size = 0;    /* no data at this point */

  /* curl's header return */
  struct MemoryStruct return_headers;
  return_headers.memory = malloc(1);  /* will be grown as needed by the realloc above */
  return_headers.size = 0;    /* no data at this point */

  /* Always disable signals */
  curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);

  /* specify URL to get */
  curl_easy_setopt(curl_handle, CURLOPT_URL, (char *)URL);

  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

  /* we pass our 'return_body' struct to the callback function */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&return_body);

  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "ydb/libcurl/1.0");

  /* Method (GET, PUT, etc.) */
  if (strlen((char *)method))
  {
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, (char *) method);
  }

  /* Payload for PUT or POST */
  if (argc >= 5) {
    if (payload->length == 0) {
      curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS,    "");
    } else {
      curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, payload->length);
      curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS,    payload->address);
    }
  }

  /* Mime type */
  if (argc >= 6 && strlen((char *)mime))
  {
    char header[100];
    char* content_type="Content-Type: ";
    int content_type_len=strlen(content_type);
    strcpy(header, content_type);
    strncat(header, (char *)mime, 100 - content_type_len - 1); /* -1 for null term */
    hs = curl_slist_append(hs, header);
  }
  if (hs) curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, hs);

  /* Timeout */
  if (argc >= 7 && timeout)
  {
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT_MS, timeout);
  }

  /* Output headers */
  if (argc >= 8)
  {
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &return_headers);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_callback);
  }

  /* get it! */
  curl_result = curl_easy_perform(curl_handle);

  /* handle output */
  if(curl_result == CURLE_OK) {
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, http_status);
    if (output->length < return_body.size)
    {
      fprintf(stderr, "Web Service return greater than GTM/YDB Max String Size %ld", output->length);
      curl_result = 255;
    }
    else if (return_headers.size && output_headers->length < return_headers.size) {
      fprintf(stderr, "Headers longer that max header size %ld", output_headers->length);
      curl_result = 255;
    }
    else
    {
      output->length = return_body.size;
      memcpy(output->address, return_body.memory, return_body.size);
      if (return_headers.size) {
        output_headers->length  = return_headers.size;
        memcpy(output_headers->address, return_headers.memory, return_headers.size);
      }
    }
  }
  else {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(curl_result));
  }

  /* setting freed mem to NULL isn't necessary, but it will help me avoid
   * freeing already freed memory */
  /* Plus we figure out if we have header by seeing if it is null or not */
  if (hs) {
    curl_slist_free_all(hs);
    hs = NULL;
  }

  /* Free return and headers buffers */
  free(return_body.memory);
  return_body.memory = NULL;
  free(return_headers.memory);
  return_headers.memory = NULL;

  /* Re-initialize (for second or later call) */
  curl_easy_reset(curl_handle);

  return (gtm_status_t)curl_result;
}

/* Easy wrapper that does everything all in one step */
gtm_status_t curl(int argc, gtm_long_t* http_status, gtm_string_t *output,
    gtm_char_t *method, gtm_char_t *URL, gtm_string_t *payload,
    gtm_char_t *mime, gtm_long_t timeout, gtm_string_t *output_headers)
{
  curl_init();
  gtm_status_t curl_result = curl_do(argc, http_status, output, method, URL, payload,
      mime, timeout, output_headers);
  curl_cleanup();
  return curl_result;
}

int main() /* tester routine to make sure everything still works */
{
  size_t output_size = 32768;
  gtm_string_t output;
  gtm_string_t payload;
  gtm_char_t mime;
  gtm_long_t timeout = 0;
  gtm_long_t http_status = 0;
  gtm_string_t output_headers;

  output.address = (char *)malloc(output_size);
  output.length = 0;
  curl_init();
  gtm_status_t status = curl_do(3, &http_status, &output, "GET",
      "https://www.example.com", &payload, &mime, timeout, &output_headers);
  curl_cleanup();
  printf("%s",output.address);
  printf("%d",status);
}
