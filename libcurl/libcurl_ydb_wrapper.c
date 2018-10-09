/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2018, Daniel Stenberg, <daniel@haxx.se>, et al.
 * Copyright (C) 2018 Sam Habiel - Modifications to work on GT.M/YDB
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

CURL *curl_handle;
struct curl_slist *hs;

gtm_status_t curl_init()
{
  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = curl_easy_init();
  
  return (gtm_status_t)0;
}

gtm_status_t curl_cleanup()
{
  /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);

  /* we're done with libcurl, so clean it up */
  curl_global_cleanup();

  return (gtm_status_t)0;
}

gtm_status_t curl_do(int argc, gtm_long_t* http_status, gtm_string_t *output, gtm_char_t *method, gtm_char_t *URL, gtm_string_t *payload, gtm_char_t *mime, gtm_long_t timeout, gtm_string_t *output_headers, gtm_string_t *input_headers)
{
  CURLcode res;

  /* Must have at least 4 arguments */
  if(argc < 4) return (gtm_status_t)-1;

  struct MemoryStruct chunk;

  chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
  chunk.size = 0;    /* no data at this point */

  /* Always disable signals */
  curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);

  /* specify URL to get */
  curl_easy_setopt(curl_handle, CURLOPT_URL, (char *)URL);

  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

  /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  
  /* Method (GET, PUT, etc.) */
  if (strlen((char *)method))
  {
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, (char *) method);
  }

  /* Payload for PUT or POST */
  if (argc >= 5 && payload->length)
  {
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, payload->length);
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS,    payload->address);
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
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, hs);
  }

  /* Timeout */
  /*
  if (timeout)
  {
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
  }
  */

  /* get it! */
  res = curl_easy_perform(curl_handle);

  /* check for errors */
  if(res == CURLE_OK) {
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, http_status);
    output->length = chunk.size;
    memcpy(output->address, chunk.memory, chunk.size);
  }
  else {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    return (gtm_status_t)-1;
  }

  if (hs) curl_slist_free_all(hs);
  
  free(chunk.memory);

  return (gtm_status_t)0;
}

gtm_status_t curl(int argc, gtm_long_t* http_status, gtm_string_t *output, gtm_char_t *method, gtm_char_t *URL, gtm_string_t *payload, gtm_char_t *mime, gtm_long_t timeout, gtm_string_t *output_headers, gtm_string_t *input_headers)
{ 
  curl_init();
  curl_do(argc, http_status, output, method, URL, payload, mime, timeout, output_headers, input_headers);
  curl_cleanup();
  return (gtm_status_t)0;
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
  gtm_string_t input_headers;
 
  output.address = (char *)malloc(output_size);
  output.length = 0;
  curl_init();
  gtm_status_t status = curl_do(3, &http_status, &output, "GET", "https://www.example.com", &payload, &mime, timeout, &output_headers, &input_headers);
  curl_cleanup();
  printf("%s",output.address);
  printf("%d",status);
}
