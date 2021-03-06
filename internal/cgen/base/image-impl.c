// After editing this file, run "go generate" in the parent directory.

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

// ---------------- Images

const uint32_t wuffs_base__pixel_format__bits_per_channel[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x0A, 0x0C, 0x10, 0x18, 0x20, 0x30, 0x40,
};

static inline uint32_t  //
wuffs_base__swap_u32_argb_abgr(uint32_t u) {
  uint32_t o = u & 0xFF00FF00;
  uint32_t r = u & 0x00FF0000;
  uint32_t b = u & 0x000000FF;
  return o | (r >> 16) | (b << 16);
}

static inline uint32_t  //
wuffs_base__composite_premul_nonpremul_u32_axxx(uint32_t dst_premul,
                                                uint32_t src_nonpremul) {
  // Convert from 8-bit color to 16-bit color.
  uint32_t sa = 0x101 * (0xFF & (src_nonpremul >> 24));
  uint32_t sr = 0x101 * (0xFF & (src_nonpremul >> 16));
  uint32_t sg = 0x101 * (0xFF & (src_nonpremul >> 8));
  uint32_t sb = 0x101 * (0xFF & (src_nonpremul >> 0));
  uint32_t da = 0x101 * (0xFF & (dst_premul >> 24));
  uint32_t dr = 0x101 * (0xFF & (dst_premul >> 16));
  uint32_t dg = 0x101 * (0xFF & (dst_premul >> 8));
  uint32_t db = 0x101 * (0xFF & (dst_premul >> 0));

  // Calculate the inverse of the src-alpha: how much of the dst to keep.
  uint32_t ia = 0xFFFF - sa;

  // Composite src (nonpremul) over dst (premul).
  da = sa + ((da * ia) / 0xFFFF);
  dr = ((sr * sa) + (dr * ia)) / 0xFFFF;
  dg = ((sg * sa) + (dg * ia)) / 0xFFFF;
  db = ((sb * sa) + (db * ia)) / 0xFFFF;

  // Convert from 16-bit color to 8-bit color and combine the components.
  da >>= 8;
  dr >>= 8;
  dg >>= 8;
  db >>= 8;
  return (db << 0) | (dg << 8) | (dr << 16) | (da << 24);
}

static inline uint32_t  //
wuffs_base__premul_u32_axxx(uint32_t nonpremul) {
  // Multiplying by 0x101 (twice, once for alpha and once for color) converts
  // from 8-bit to 16-bit color. Shifting right by 8 undoes that.
  //
  // Working in the higher bit depth can produce slightly different (and
  // arguably slightly more accurate) results. For example, given 8-bit blue
  // and alpha of 0x80 and 0x81:
  //
  //  - ((0x80   * 0x81  ) / 0xFF  )      = 0x40        = 0x40
  //  - ((0x8080 * 0x8181) / 0xFFFF) >> 8 = 0x4101 >> 8 = 0x41
  uint32_t a = 0xFF & (nonpremul >> 24);
  uint32_t a16 = a * (0x101 * 0x101);

  uint32_t r = 0xFF & (nonpremul >> 16);
  r = ((r * a16) / 0xFFFF) >> 8;
  uint32_t g = 0xFF & (nonpremul >> 8);
  g = ((g * a16) / 0xFFFF) >> 8;
  uint32_t b = 0xFF & (nonpremul >> 0);
  b = ((b * a16) / 0xFFFF) >> 8;

  return (a << 24) | (r << 16) | (g << 8) | (b << 0);
}

static inline uint32_t  //
wuffs_base__nonpremul_u32_axxx(uint32_t premul) {
  uint32_t a = 0xFF & (premul >> 24);
  if (a == 0xFF) {
    return premul;
  } else if (a == 0) {
    return 0;
  }
  uint32_t a16 = a * 0x101;

  uint32_t r = 0xFF & (premul >> 16);
  r = ((r * (0x101 * 0xFFFF)) / a16) >> 8;
  uint32_t g = 0xFF & (premul >> 8);
  g = ((g * (0x101 * 0xFFFF)) / a16) >> 8;
  uint32_t b = 0xFF & (premul >> 0);
  b = ((b * (0x101 * 0xFFFF)) / a16) >> 8;

  return (a << 24) | (r << 16) | (g << 8) | (b << 0);
}

wuffs_base__color_u32_argb_premul  //
wuffs_base__pixel_buffer__color_u32_at(const wuffs_base__pixel_buffer* b,
                                       uint32_t x,
                                       uint32_t y) {
  if (!b || (x >= b->pixcfg.private_impl.width) ||
      (y >= b->pixcfg.private_impl.height)) {
    return 0;
  }

  if (wuffs_base__pixel_format__is_planar(&b->pixcfg.private_impl.pixfmt)) {
    // TODO: support planar formats.
    return 0;
  }

  size_t stride = b->private_impl.planes[0].stride;
  uint8_t* row = b->private_impl.planes[0].ptr + (stride * ((size_t)y));

  switch (b->pixcfg.private_impl.pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
      WUFFS_BASE__FALLTHROUGH;
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
      return wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x)));

    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_PREMUL:
      WUFFS_BASE__FALLTHROUGH;
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY: {
      uint8_t* palette = b->private_impl.planes[3].ptr;
      return wuffs_base__load_u32le__no_bounds_check(palette +
                                                     (4 * ((size_t)row[x])));
    }

      // Common formats above. Rarer formats below.

    case WUFFS_BASE__PIXEL_FORMAT__Y:
      return 0xFF000000 | (0x00010101 * ((uint32_t)(row[x])));

    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL: {
      uint8_t* palette = b->private_impl.planes[3].ptr;
      return wuffs_base__premul_u32_axxx(
          wuffs_base__load_u32le__no_bounds_check(palette +
                                                  (4 * ((size_t)row[x]))));
    }

    case WUFFS_BASE__PIXEL_FORMAT__BGR_565: {
      uint16_t bgr =
          wuffs_base__load_u16le__no_bounds_check(row + (2 * ((size_t)x)));
      uint32_t b5 = 0x1F & (bgr >> 0);
      uint32_t b = (b5 << 3) | (b5 >> 2);
      uint32_t g6 = 0x3F & (bgr >> 5);
      uint32_t g = (g6 << 2) | (g6 >> 4);
      uint32_t r5 = 0x1F & (bgr >> 11);
      uint32_t r = (r5 << 3) | (r5 >> 2);
      return 0xFF000000 | (r << 16) | (g << 8) | (b << 0);
    }
    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      return 0xFF000000 |
             wuffs_base__load_u24le__no_bounds_check(row + (3 * ((size_t)x)));
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
      return 0xFF000000 |
             wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x)));
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      return wuffs_base__premul_u32_axxx(
          wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x))));

    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      return wuffs_base__swap_u32_argb_abgr(
          0xFF000000 |
          wuffs_base__load_u24le__no_bounds_check(row + (3 * ((size_t)x))));
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
      return wuffs_base__swap_u32_argb_abgr(
          0xFF000000 |
          wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x))));
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
      return wuffs_base__swap_u32_argb_abgr(wuffs_base__premul_u32_axxx(
          wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x)))));
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
      WUFFS_BASE__FALLTHROUGH;
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
      return wuffs_base__swap_u32_argb_abgr(
          wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x))));

    default:
      // TODO: support more formats.
      break;
  }

  return 0;
}

wuffs_base__status  //
wuffs_base__pixel_buffer__set_color_u32_at(
    wuffs_base__pixel_buffer* b,
    uint32_t x,
    uint32_t y,
    wuffs_base__color_u32_argb_premul color) {
  if (!b) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if ((x >= b->pixcfg.private_impl.width) ||
      (y >= b->pixcfg.private_impl.height)) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }

  if (wuffs_base__pixel_format__is_planar(&b->pixcfg.private_impl.pixfmt)) {
    // TODO: support planar formats.
    return wuffs_base__make_status(wuffs_base__error__unsupported_option);
  }

  size_t stride = b->private_impl.planes[0].stride;
  uint8_t* row = b->private_impl.planes[0].ptr + (stride * ((size_t)y));

  switch (b->pixcfg.private_impl.pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
      WUFFS_BASE__FALLTHROUGH;
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
      wuffs_base__store_u32le__no_bounds_check(row + (4 * ((size_t)x)), color);
      break;

      // Common formats above. Rarer formats below.

    case WUFFS_BASE__PIXEL_FORMAT__BGR_565: {
      uint32_t b5 = 0x1F & (color >> (8 - 5));
      uint32_t g6 = 0x3F & (color >> (16 - 6));
      uint32_t r5 = 0x1F & (color >> (24 - 5));
      uint32_t bgr565 = (b5 << 0) | (g6 << 5) | (r5 << 11);
      wuffs_base__store_u16le__no_bounds_check(row + (2 * ((size_t)x)),
                                               (uint16_t)bgr565);
      break;
    }
    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      wuffs_base__store_u24le__no_bounds_check(row + (3 * ((size_t)x)), color);
      break;
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      wuffs_base__store_u32le__no_bounds_check(
          row + (4 * ((size_t)x)), wuffs_base__nonpremul_u32_axxx(color));
      break;

    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      wuffs_base__store_u24le__no_bounds_check(
          row + (3 * ((size_t)x)), wuffs_base__swap_u32_argb_abgr(color));
      break;
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
      wuffs_base__store_u32le__no_bounds_check(
          row + (4 * ((size_t)x)), wuffs_base__nonpremul_u32_axxx(
                                       wuffs_base__swap_u32_argb_abgr(color)));
      break;
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
      WUFFS_BASE__FALLTHROUGH;
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
      wuffs_base__store_u32le__no_bounds_check(
          row + (4 * ((size_t)x)), wuffs_base__swap_u32_argb_abgr(color));
      break;

    default:
      // TODO: support more formats.
      return wuffs_base__make_status(wuffs_base__error__unsupported_option);
  }

  return wuffs_base__make_status(NULL);
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__bgra_premul__bgra_nonpremul__src(
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  size_t dst_len4 = dst.len / 4;
  size_t src_len4 = src.len / 4;
  size_t len = dst_len4 < src_len4 ? dst_len4 : src_len4;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t s0 = wuffs_base__premul_u32_axxx(
        wuffs_base__load_u32le__no_bounds_check(s + (0 * 4)));
    wuffs_base__store_u32le__no_bounds_check(d + (0 * 4), s0);

    s += 1 * 4;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgra_premul__bgra_nonpremul__src_over(
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  size_t dst_len4 = dst.len / 4;
  size_t src_len4 = src.len / 4;
  size_t len = dst_len4 < src_len4 ? dst_len4 : src_len4;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t d0 = wuffs_base__load_u32le__no_bounds_check(d + (0 * 4));
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(s + (0 * 4));
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4), wuffs_base__composite_premul_nonpremul_u32_axxx(d0, s0));

    s += 1 * 4;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__copy_1_1(wuffs_base__slice_u8 dst,
                                     wuffs_base__slice_u8 dst_palette,
                                     wuffs_base__slice_u8 src) {
  return wuffs_base__slice_u8__copy_from_slice(dst, src);
}

static uint64_t  //
wuffs_base__pixel_swizzler__copy_4_4(wuffs_base__slice_u8 dst,
                                     wuffs_base__slice_u8 dst_palette,
                                     wuffs_base__slice_u8 src) {
  size_t dst_len4 = dst.len / 4;
  size_t src_len4 = src.len / 4;
  size_t len = dst_len4 < src_len4 ? dst_len4 : src_len4;
  if (len > 0) {
    memmove(dst.ptr, src.ptr, len * 4);
  }
  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xx__index__src(wuffs_base__slice_u8 dst,
                                           wuffs_base__slice_u8 dst_palette,
                                           wuffs_base__slice_u8 src) {
  if (dst_palette.len != 1024) {
    return 0;
  }
  size_t dst_len2 = dst.len / 2;
  size_t len = dst_len2 < src.len ? dst_len2 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    wuffs_base__store_u16le__no_bounds_check(
        d + (0 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[0] * 4)));
    wuffs_base__store_u16le__no_bounds_check(
        d + (1 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[1] * 4)));
    wuffs_base__store_u16le__no_bounds_check(
        d + (2 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[2] * 4)));
    wuffs_base__store_u16le__no_bounds_check(
        d + (3 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[3] * 4)));

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 2;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    wuffs_base__store_u16le__no_bounds_check(
        d + (0 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[0] * 4)));

    s += 1 * 1;
    d += 1 * 2;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxx__index__src(wuffs_base__slice_u8 dst,
                                            wuffs_base__slice_u8 dst_palette,
                                            wuffs_base__slice_u8 src) {
  if (dst_palette.len != 1024) {
    return 0;
  }
  size_t dst_len3 = dst.len / 3;
  size_t len = dst_len3 < src.len ? dst_len3 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  // The comparison in the while condition is ">", not ">=", because with ">=",
  // the last 4-byte store could write past the end of the dst slice.
  //
  // Each 4-byte store writes one too many bytes, but a subsequent store will
  // overwrite that with the correct byte. There is always another store,
  // whether a 4-byte store in this loop or a 1-byte store in the next loop.
  while (n > loop_unroll_count) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 3), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[0] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (1 * 3), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[1] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (2 * 3), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[2] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (3 * 3), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[3] * 4)));

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 3;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[0] * 4));
    d[0] = (uint8_t)(s0 >> 0);
    d[1] = (uint8_t)(s0 >> 8);
    d[2] = (uint8_t)(s0 >> 16);

    s += 1 * 1;
    d += 1 * 3;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxx__index_binary_alpha__src_over(
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  if (dst_palette.len != 1024) {
    return 0;
  }
  size_t dst_len3 = dst.len / 3;
  size_t len = dst_len3 < src.len ? dst_len3 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u24le__no_bounds_check(d + (0 * 4), s0);
    }
    uint32_t s1 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[1] * 4));
    if (s1) {
      wuffs_base__store_u24le__no_bounds_check(d + (1 * 4), s1);
    }
    uint32_t s2 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[2] * 4));
    if (s2) {
      wuffs_base__store_u24le__no_bounds_check(d + (2 * 4), s2);
    }
    uint32_t s3 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[3] * 4));
    if (s3) {
      wuffs_base__store_u24le__no_bounds_check(d + (3 * 4), s3);
    }

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 3;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u24le__no_bounds_check(d + (0 * 4), s0);
    }

    s += 1 * 1;
    d += 1 * 3;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxxx__index__src(wuffs_base__slice_u8 dst,
                                             wuffs_base__slice_u8 dst_palette,
                                             wuffs_base__slice_u8 src) {
  if (dst_palette.len != 1024) {
    return 0;
  }
  size_t dst_len4 = dst.len / 4;
  size_t len = dst_len4 < src.len ? dst_len4 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[0] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (1 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[1] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (2 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[2] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (3 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[3] * 4)));

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 4;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[0] * 4)));

    s += 1 * 1;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxxx__index_binary_alpha__src_over(
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  if (dst_palette.len != 1024) {
    return 0;
  }
  size_t dst_len4 = dst.len / 4;
  size_t len = dst_len4 < src.len ? dst_len4 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u32le__no_bounds_check(d + (0 * 4), s0);
    }
    uint32_t s1 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[1] * 4));
    if (s1) {
      wuffs_base__store_u32le__no_bounds_check(d + (1 * 4), s1);
    }
    uint32_t s2 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[2] * 4));
    if (s2) {
      wuffs_base__store_u32le__no_bounds_check(d + (2 * 4), s2);
    }
    uint32_t s3 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[3] * 4));
    if (s3) {
      wuffs_base__store_u32le__no_bounds_check(d + (3 * 4), s3);
    }

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 4;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u32le__no_bounds_check(d + (0 * 4), s0);
    }

    s += 1 * 1;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxxx__xxx(wuffs_base__slice_u8 dst,
                                      wuffs_base__slice_u8 dst_palette,
                                      wuffs_base__slice_u8 src) {
  size_t dst_len4 = dst.len / 4;
  size_t src_len3 = src.len / 3;
  size_t len = dst_len4 < src_len3 ? dst_len4 : src_len3;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4),
        0xFF000000 | wuffs_base__load_u24le__no_bounds_check(s + (0 * 3)));

    s += 1 * 3;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxxx__y(wuffs_base__slice_u8 dst,
                                    wuffs_base__slice_u8 dst_palette,
                                    wuffs_base__slice_u8 src) {
  size_t dst_len4 = dst.len / 4;
  size_t len = dst_len4 < src.len ? dst_len4 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4), 0xFF000000 | (0x010101 * (uint32_t)s[0]));

    s += 1 * 1;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__squash_bgr_565_888(wuffs_base__slice_u8 dst,
                                               wuffs_base__slice_u8 src) {
  size_t len4 = (dst.len < src.len ? dst.len : src.len) / 4;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;

  size_t n = len4;
  while (n--) {
    uint32_t argb = wuffs_base__load_u32le__no_bounds_check(s);
    uint32_t b5 = 0x1F & (argb >> (8 - 5));
    uint32_t g6 = 0x3F & (argb >> (16 - 6));
    uint32_t r5 = 0x1F & (argb >> (24 - 5));
    wuffs_base__store_u32le__no_bounds_check(
        d, (b5 << 0) | (g6 << 5) | (r5 << 11));
    s += 4;
    d += 4;
  }
  return len4 * 4;
}

static uint64_t  //
wuffs_base__pixel_swizzler__swap_rgbx_bgrx(wuffs_base__slice_u8 dst,
                                           wuffs_base__slice_u8 src) {
  size_t len4 = (dst.len < src.len ? dst.len : src.len) / 4;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;

  size_t n = len4;
  while (n--) {
    uint8_t b0 = s[0];
    uint8_t b1 = s[1];
    uint8_t b2 = s[2];
    uint8_t b3 = s[3];
    d[0] = b2;
    d[1] = b1;
    d[2] = b0;
    d[3] = b3;
    s += 4;
    d += 4;
  }
  return len4 * 4;
}

// --------

static wuffs_base__pixel_swizzler__func  //
wuffs_base__pixel_swizzler__prepare__y(wuffs_base__pixel_swizzler* p,
                                       wuffs_base__pixel_format dst_format,
                                       wuffs_base__slice_u8 dst_palette,
                                       wuffs_base__slice_u8 src_palette,
                                       wuffs_base__pixel_blend blend) {
  switch (dst_format.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      // TODO.
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      // TODO.
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
      return wuffs_base__pixel_swizzler__xxxx__y;
  }
  return NULL;
}

static wuffs_base__pixel_swizzler__func  //
wuffs_base__pixel_swizzler__prepare__indexed__bgra_binary(
    wuffs_base__pixel_swizzler* p,
    wuffs_base__pixel_format dst_format,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src_palette,
    wuffs_base__pixel_blend blend) {
  switch (dst_format.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY:
      if (wuffs_base__slice_u8__copy_from_slice(dst_palette, src_palette) !=
          1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__copy_1_1;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      if (wuffs_base__pixel_swizzler__squash_bgr_565_888(dst_palette,
                                                         src_palette) != 1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__xx__index__src;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      if (wuffs_base__slice_u8__copy_from_slice(dst_palette, src_palette) !=
          1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__xxx__index__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__xxx__index_binary_alpha__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
      if (wuffs_base__slice_u8__copy_from_slice(dst_palette, src_palette) !=
          1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__xxxx__index__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__xxxx__index_binary_alpha__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      if (wuffs_base__pixel_swizzler__swap_rgbx_bgrx(dst_palette,
                                                     src_palette) != 1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__xxx__index__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__xxx__index_binary_alpha__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
      if (wuffs_base__pixel_swizzler__swap_rgbx_bgrx(dst_palette,
                                                     src_palette) != 1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__xxxx__index__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__xxxx__index_binary_alpha__src_over;
      }
      return NULL;
  }
  return NULL;
}

static wuffs_base__pixel_swizzler__func  //
wuffs_base__pixel_swizzler__prepare__bgr(wuffs_base__pixel_swizzler* p,
                                         wuffs_base__pixel_format dst_format,
                                         wuffs_base__slice_u8 dst_palette,
                                         wuffs_base__slice_u8 src_palette,
                                         wuffs_base__pixel_blend blend) {
  switch (dst_format.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      // TODO.
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      // TODO.
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
      return wuffs_base__pixel_swizzler__xxxx__xxx;

    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
      // TODO.
      break;
  }
  return NULL;
}

static wuffs_base__pixel_swizzler__func  //
wuffs_base__pixel_swizzler__prepare__bgra_nonpremul(
    wuffs_base__pixel_swizzler* p,
    wuffs_base__pixel_format dst_format,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src_palette,
    wuffs_base__pixel_blend blend) {
  switch (dst_format.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      // TODO.
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      // TODO.
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__copy_4_4;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__bgra_premul__bgra_nonpremul__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__bgra_premul__bgra_nonpremul__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
      // TODO.
      break;

    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
      // TODO.
      break;
  }
  return NULL;
}

// --------

wuffs_base__status  //
wuffs_base__pixel_swizzler__prepare(wuffs_base__pixel_swizzler* p,
                                    wuffs_base__pixel_format dst_format,
                                    wuffs_base__slice_u8 dst_palette,
                                    wuffs_base__pixel_format src_format,
                                    wuffs_base__slice_u8 src_palette,
                                    wuffs_base__pixel_blend blend) {
  if (!p) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }

  // TODO: support many more formats.

  wuffs_base__pixel_swizzler__func func = NULL;

  switch (src_format.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__Y:
      func = wuffs_base__pixel_swizzler__prepare__y(p, dst_format, dst_palette,
                                                    src_palette, blend);
      break;

    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY:
      func = wuffs_base__pixel_swizzler__prepare__indexed__bgra_binary(
          p, dst_format, dst_palette, src_palette, blend);
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      func = wuffs_base__pixel_swizzler__prepare__bgr(
          p, dst_format, dst_palette, src_palette, blend);
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      func = wuffs_base__pixel_swizzler__prepare__bgra_nonpremul(
          p, dst_format, dst_palette, src_palette, blend);
      break;
  }

  p->private_impl.func = func;
  return wuffs_base__make_status(
      func ? NULL : wuffs_base__error__unsupported_pixel_swizzler_option);
}

uint64_t  //
wuffs_base__pixel_swizzler__swizzle_interleaved(
    const wuffs_base__pixel_swizzler* p,
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  if (p && p->private_impl.func) {
    return (*p->private_impl.func)(dst, dst_palette, src);
  }
  return 0;
}
