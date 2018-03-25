// Copyright 2017 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Uncomment this line to test and bench miniz instead of zlib-the-library.
// #define WUFFS_MIMICLIB_USE_MINIZ_INSTEAD_OF_ZLIB 1

#ifdef WUFFS_MIMICLIB_USE_MINIZ_INSTEAD_OF_ZLIB
#include "/path/to/your/copy/of/github.com/richgel999/miniz/miniz_tinfl.c"

const char* mimic_bench_adler32(wuffs_base__buf1* dst,
                                wuffs_base__buf1* src,
                                uint64_t wlimit,
                                uint64_t rlimit) {
  return "miniz does not independently compute Adler32";
}

const char* mimic_bench_crc32_ieee(wuffs_base__buf1* dst,
                                   wuffs_base__buf1* src,
                                   uint64_t wlimit,
                                   uint64_t rlimit) {
  return "miniz does not implement CRC32/IEEE";
}

const char* mimic_deflate_zlib_decode(wuffs_base__buf1* dst,
                                      wuffs_base__buf1* src,
                                      uint64_t wlimit,
                                      uint64_t rlimit,
                                      bool deflate_instead_of_zlib) {
  // TODO: don't ignore wlimit and rlimit.
  int flags = 0;
  if (!deflate_instead_of_zlib) {
    flags |= TINFL_FLAG_PARSE_ZLIB_HEADER;
  }
  size_t n =
      tinfl_decompress_mem_to_mem(dst->ptr + dst->wi, dst->len - dst->wi,
                                  src->ptr + src->ri, src->wi - src->ri, flags);
  if (n == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED) {
    return "TINFL_DECOMPRESS_MEM_TO_MEM_FAILED";
  }
  dst->wi += n;
  src->ri = src->wi;
  return NULL;
}

const char* mimic_deflate_decode(wuffs_base__buf1* dst,
                                 wuffs_base__buf1* src,
                                 uint64_t wlimit,
                                 uint64_t rlimit) {
  return mimic_deflate_zlib_decode(dst, src, wlimit, rlimit, true);
}

const char* mimic_gzip_decode(wuffs_base__buf1* dst,
                              wuffs_base__buf1* src,
                              uint64_t wlimit,
                              uint64_t rlimit) {
  return "miniz does not implement gzip";
}

const char* mimic_zlib_decode(wuffs_base__buf1* dst,
                              wuffs_base__buf1* src,
                              uint64_t wlimit,
                              uint64_t rlimit) {
  return mimic_deflate_zlib_decode(dst, src, wlimit, rlimit, false);
}

#else  // WUFFS_MIMICLIB_USE_MINIZ_INSTEAD_OF_ZLIB
#include "zlib.h"

uint32_t global_mimiclib_deflate_unused_u32;

const char* mimic_bench_adler32(wuffs_base__buf1* dst,
                                wuffs_base__buf1* src,
                                uint64_t wlimit,
                                uint64_t rlimit) {
  // TODO: don't ignore wlimit and rlimit.
  uint8_t* ptr = src->ptr + src->ri;
  size_t len = src->wi - src->ri;
  if (len > 0x7FFFFFFF) {
    return "src length is too large";
  }
  global_mimiclib_deflate_unused_u32 = adler32(0L, ptr, len);
  src->ri = src->wi;
  return NULL;
}

const char* mimic_bench_crc32_ieee(wuffs_base__buf1* dst,
                                   wuffs_base__buf1* src,
                                   uint64_t wlimit,
                                   uint64_t rlimit) {
  // TODO: don't ignore wlimit and rlimit.
  uint8_t* ptr = src->ptr + src->ri;
  size_t len = src->wi - src->ri;
  if (len > 0x7FFFFFFF) {
    return "src length is too large";
  }
  global_mimiclib_deflate_unused_u32 = crc32(0L, ptr, len);
  src->ri = src->wi;
  return NULL;
}

typedef enum {
  zlib_flavor_raw,
  zlib_flavor_gzip,
  zlib_flavor_zlib,
} zlib_flavor;

const char* mimic_deflate_gzip_zlib_decode(wuffs_base__buf1* dst,
                                           wuffs_base__buf1* src,
                                           uint64_t wlimit,
                                           uint64_t rlimit,
                                           zlib_flavor flavor) {
  // TODO: don't ignore wlimit and rlimit.
  const char* ret = NULL;

  // See inflateInit2 in the zlib manual, or in zlib.h, for details about how
  // the window_bits int also encodes the wire format wrapper.
  int window_bits = 0;
  switch (flavor) {
    case zlib_flavor_raw:
      window_bits = -15;
      break;
    case zlib_flavor_gzip:
      window_bits = +15 | 16;
      break;
    case zlib_flavor_zlib:
      window_bits = +15;
      break;
    default:
      ret = "invalid zlib_flavor";
      goto cleanup0;
  }
  z_stream z = {0};
  int ii2_err = inflateInit2(&z, window_bits);
  if (ii2_err != Z_OK) {
    ret = "inflateInit2 failed";
    goto cleanup0;
  }

  z.avail_in = src->wi - src->ri;
  z.next_in = src->ptr + src->ri;
  z.avail_out = dst->len - dst->wi;
  z.next_out = dst->ptr + dst->wi;

  int i_err = inflate(&z, Z_NO_FLUSH);
  if (i_err != Z_STREAM_END) {
    ret = "inflate failed";
    goto cleanup1;
  }

  size_t readable = src->wi - src->ri;
  size_t r_remaining = z.avail_in;
  if (readable < r_remaining) {
    ret = "inconsistent avail_in";
    goto cleanup1;
  }
  src->ri += readable - r_remaining;

  size_t writable = dst->len - dst->wi;
  size_t w_remaining = z.avail_out;
  if (writable < w_remaining) {
    ret = "inconsistent avail_out";
    goto cleanup1;
  }
  dst->wi += writable - w_remaining;

cleanup1:;
  int ie_err = inflateEnd(&z);
  if ((ie_err != Z_OK) && !ret) {
    ret = "inflateEnd failed";
  }

cleanup0:;
  return ret;
}

const char* mimic_deflate_decode(wuffs_base__buf1* dst,
                                 wuffs_base__buf1* src,
                                 uint64_t wlimit,
                                 uint64_t rlimit) {
  return mimic_deflate_gzip_zlib_decode(dst, src, wlimit, rlimit,
                                        zlib_flavor_raw);
}

const char* mimic_gzip_decode(wuffs_base__buf1* dst,
                              wuffs_base__buf1* src,
                              uint64_t wlimit,
                              uint64_t rlimit) {
  return mimic_deflate_gzip_zlib_decode(dst, src, wlimit, rlimit,
                                        zlib_flavor_gzip);
}

const char* mimic_zlib_decode(wuffs_base__buf1* dst,
                              wuffs_base__buf1* src,
                              uint64_t wlimit,
                              uint64_t rlimit) {
  return mimic_deflate_gzip_zlib_decode(dst, src, wlimit, rlimit,
                                        zlib_flavor_zlib);
}

#endif  // WUFFS_MIMICLIB_USE_MINIZ_INSTEAD_OF_ZLIB
