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

// ---------------- I/O

struct wuffs_base__io_buffer__struct;

typedef struct {
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    struct wuffs_base__io_buffer__struct* buf;
    // The bounds values are typically NULL, when created by the Wuffs public
    // API. NULL means that the callee substitutes the implicit bounds derived
    // from buf.
    uint8_t* mark;
    uint8_t* limit;
  } private_impl;
} wuffs_base__io_reader;

typedef struct {
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    struct wuffs_base__io_buffer__struct* buf;
    // The bounds values are typically NULL, when created by the Wuffs public
    // API. NULL means that the callee substitutes the implicit bounds derived
    // from buf.
    uint8_t* mark;
    uint8_t* limit;
  } private_impl;
} wuffs_base__io_writer;

// wuffs_base__io_buffer_meta is the metadata for a wuffs_base__io_buffer's
// data.
typedef struct {
  size_t wi;     // Write index. Invariant: wi <= len.
  size_t ri;     // Read  index. Invariant: ri <= wi.
  uint64_t pos;  // Position of the buffer start relative to the stream start.
  bool closed;   // No further writes are expected.
} wuffs_base__io_buffer_meta;

// wuffs_base__io_buffer is a 1-dimensional buffer (a pointer and length) plus
// additional metadata.
//
// A value with all fields zero is a valid, empty buffer.
typedef struct wuffs_base__io_buffer__struct {
  wuffs_base__slice_u8 data;
  wuffs_base__io_buffer_meta meta;

#ifdef __cplusplus
  inline void compact();
  inline wuffs_base__io_reader reader();
  inline wuffs_base__io_writer writer();
  inline uint64_t reader_io_position();
  inline uint64_t writer_io_position();
#endif  // __cplusplus

} wuffs_base__io_buffer;

static inline wuffs_base__io_buffer  //
wuffs_base__make_io_buffer(wuffs_base__slice_u8 data,
                           wuffs_base__io_buffer_meta meta) {
  wuffs_base__io_buffer ret;
  ret.data = data;
  ret.meta = meta;
  return ret;
}

static inline wuffs_base__io_buffer_meta  //
wuffs_base__make_io_buffer_meta(size_t wi,
                                size_t ri,
                                uint64_t pos,
                                bool closed) {
  wuffs_base__io_buffer_meta ret;
  ret.wi = wi;
  ret.ri = ri;
  ret.pos = pos;
  ret.closed = closed;
  return ret;
}

static inline wuffs_base__io_buffer  //
wuffs_base__null_io_buffer() {
  wuffs_base__io_buffer ret;
  ret.data.ptr = NULL;
  ret.data.len = 0;
  ret.meta.wi = 0;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = false;
  return ret;
}

static inline wuffs_base__io_buffer_meta  //
wuffs_base__null_io_buffer_meta() {
  wuffs_base__io_buffer_meta ret;
  ret.wi = 0;
  ret.ri = 0;
  ret.pos = 0;
  ret.closed = false;
  return ret;
}

static inline wuffs_base__io_reader  //
wuffs_base__null_io_reader() {
  wuffs_base__io_reader ret;
  ret.private_impl.buf = NULL;
  ret.private_impl.mark = NULL;
  ret.private_impl.limit = NULL;
  return ret;
}

static inline wuffs_base__io_writer  //
wuffs_base__null_io_writer() {
  wuffs_base__io_writer ret;
  ret.private_impl.buf = NULL;
  ret.private_impl.mark = NULL;
  ret.private_impl.limit = NULL;
  return ret;
}

// wuffs_base__io_buffer__compact moves any written but unread bytes to the
// start of the buffer.
static inline void  //
wuffs_base__io_buffer__compact(wuffs_base__io_buffer* buf) {
  if (!buf || (buf->meta.ri == 0)) {
    return;
  }
  buf->meta.pos = wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.ri);
  size_t n = buf->meta.wi - buf->meta.ri;
  if (n != 0) {
    memmove(buf->data.ptr, buf->data.ptr + buf->meta.ri, n);
  }
  buf->meta.wi = n;
  buf->meta.ri = 0;
}

static inline wuffs_base__io_reader  //
wuffs_base__io_buffer__reader(wuffs_base__io_buffer* buf) {
  wuffs_base__io_reader ret;
  ret.private_impl.buf = buf;
  ret.private_impl.mark = NULL;
  ret.private_impl.limit = NULL;
  return ret;
}

static inline wuffs_base__io_writer  //
wuffs_base__io_buffer__writer(wuffs_base__io_buffer* buf) {
  wuffs_base__io_writer ret;
  ret.private_impl.buf = buf;
  ret.private_impl.mark = NULL;
  ret.private_impl.limit = NULL;
  return ret;
}

static inline uint64_t  //
wuffs_base__io_buffer__reader_io_position(wuffs_base__io_buffer* buf) {
  return buf ? wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.ri) : 0;
}

static inline uint64_t  //
wuffs_base__io_buffer__writer_io_position(wuffs_base__io_buffer* buf) {
  return buf ? wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.wi) : 0;
}

#ifdef __cplusplus

inline void  //
wuffs_base__io_buffer__struct::compact() {
  wuffs_base__io_buffer__compact(this);
}

inline wuffs_base__io_reader  //
wuffs_base__io_buffer__struct::reader() {
  return wuffs_base__io_buffer__reader(this);
}

inline wuffs_base__io_writer  //
wuffs_base__io_buffer__struct::writer() {
  return wuffs_base__io_buffer__writer(this);
}

inline uint64_t  //
wuffs_base__io_buffer__struct::reader_io_position() {
  return wuffs_base__io_buffer__reader_io_position(this);
}

inline uint64_t  //
wuffs_base__io_buffer__struct::writer_io_position() {
  return wuffs_base__io_buffer__writer_io_position(this);
}

#endif  // __cplusplus
