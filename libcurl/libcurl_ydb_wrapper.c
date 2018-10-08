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

gtm_status_t curl_init(int argc)
{
  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = curl_easy_init();
  
  return (gtm_status_t)0;
}

gtm_status_t curl_cleanup(int argc)
{
  /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);

  /* we're done with libcurl, so clean it up */
  curl_global_cleanup();

  return (gtm_status_t)0;
}

gtm_status_t curl(int argc, gtm_string_t *output, gtm_char_t *method, gtm_char_t *URL, gtm_string_t *payload, gtm_char_t *mime, gtm_long_t timeout, gtm_string_t *output_headers, gtm_string_t *input_headers)
{ 
  CURLcode res;
  
  curl_init(0);

  struct MemoryStruct chunk;

  chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
  chunk.size = 0;    /* no data at this point */

  /* specify URL to get */
  curl_easy_setopt(curl_handle, CURLOPT_URL, (char *)URL);

  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

  /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  /* get it! */
  res = curl_easy_perform(curl_handle);

  /* check for errors */
  if(res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    return (gtm_status_t)-1;
  }
  else {
    
    /*
     * Now, our chunk.memory points to a memory block that is chunk.size
     * bytes big and contains the remote file.
     *
     * Do something nice with it!
     */

    output->length = chunk.size;
    memcpy(output->address, chunk.memory, chunk.size);
  }

  free(chunk.memory);

  curl_cleanup(0);

  return (gtm_status_t)0;
}

int main() /* debugger routine to make sure everything still works */
{
  size_t output_size = 32768;
  gtm_string_t output;
  gtm_string_t payload;
  gtm_char_t mime;
  gtm_long_t timeout = 0;
  gtm_string_t output_headers;
  gtm_string_t input_headers;
 
  output.address = (char *)malloc(output_size);
  output.length = 0;
  gtm_status_t status = curl(3, &output, "GET", "https://www.example.com", &payload, &mime, timeout, &output_headers, &input_headers);
  printf("%s",output.address);
  printf("%d",status);
}
