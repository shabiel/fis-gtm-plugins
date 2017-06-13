$gtm_dist/plugin/libgtmzlib.so

compress2: gtm_status_t zlib_compress2( I:gtm_string_t*, O:gtm_string_t* [1048576], I:gtm_int_t )
uncompress: gtm_status_t zlib_uncompress( I:gtm_string_t*, O:gtm_string_t* [1048576] )
zlibVersion: gtm_status_t zlib_zlibVersion( O:gtm_char_t* [256] )
