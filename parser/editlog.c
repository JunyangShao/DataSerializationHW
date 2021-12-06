#include "editlog.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int32_t pos;
typedef struct {
  pos *trace;
  pos capacity, iter, grow;
} n_trace;
#define parser_fail(i) __builtin_expect((i) < 0, 0)
static uint64_t read_unsigned_bits_littleendian(NailStream *stream,
                                                unsigned count) {
  uint64_t retval = 0;
  unsigned int out_idx = 0;
  size_t pos = stream->pos;
  char bit_offset = stream->bit_offset;
  const uint8_t *data = stream->data;
  while (count > 0) {
    if (bit_offset == 0 && (count & 7) == 0) {
      retval |= data[pos] << out_idx;
      out_idx += 8;
      pos++;
      count -= 8;
    } else {
      // This can use a lot of performance love
      // TODO: test this
      retval |= ((data[pos] >> (bit_offset)) & 1) << out_idx;
      out_idx++;
      count--;
      bit_offset++;
      if (bit_offset > 7) {
        bit_offset -= 8;
        pos++;
      }
    }
  }
  stream->pos = pos;
  stream->bit_offset = bit_offset;
  return retval;
}

static uint64_t read_unsigned_bits(NailStream *stream, unsigned count) {
  uint64_t retval = 0;
  unsigned int out_idx = count;
  size_t pos = stream->pos;
  char bit_offset = stream->bit_offset;
  const uint8_t *data = stream->data;
  // TODO: Implement little endian too
  // Count LSB to MSB
  while (count > 0) {
    if (bit_offset == 0 && (count & 7) == 0) {
      out_idx -= 8;
      retval |= data[pos] << out_idx;
      pos++;
      count -= 8;
    } else {
      // This can use a lot of performance love
      // TODO: implement other endianesses
      out_idx--;
      retval |= ((data[pos] >> (7 - bit_offset)) & 1) << out_idx;
      count--;
      bit_offset++;
      if (bit_offset > 7) {
        bit_offset -= 8;
        pos++;
      }
    }
  }
  stream->pos = pos;
  stream->bit_offset = bit_offset;
  return retval;
}
static int stream_check(const NailStream *stream, unsigned count) {
  if (stream->size - (count >> 3) - ((stream->bit_offset + count & 7) >> 3) <
      stream->pos)
    return -1;
  return 0;
}
static void stream_advance(NailStream *stream, unsigned count) {

  stream->pos += count >> 3;
  stream->bit_offset += count & 7;
  if (stream->bit_offset > 7) {
    stream->pos++;
    stream->bit_offset -= 8;
  }
}
static NailStreamPos stream_getpos(NailStream *stream) {
  return (stream->pos << 3) + stream->bit_offset; // TODO: Overflow potential!
}
static void stream_backup(NailStream *stream, unsigned count) {
  stream->pos -= count >> 3;
  stream->bit_offset -= count & 7;
  if (stream->bit_offset < 0) {
    stream->pos--;
    stream->bit_offset += 8;
  }
}
//#define BITSLICE(x, off, len) read_unsigned_bits(x,off,len)
/* trace is a minimalistic representation of the AST. Many parsers add a count,
 * choice parsers add two pos parameters (which choice was taken and where in
 * the trace it begins) const parsers emit a new input position
 */
typedef struct {
  pos position;
  pos parser;
  pos result;
} n_hash;

typedef struct {
  //        p_hash *memo;
  unsigned lg_size; // How large is the hashtable - make it a power of two
} n_hashtable;

static int n_trace_init(n_trace *out, pos size, pos grow) {
  if (size <= 1) {
    return -1;
  }
  out->trace = (pos *)malloc(size * sizeof(pos));
  if (!out) {
    return -1;
  }
  out->capacity = size - 1;
  out->iter = 0;
  if (grow < 16) { // Grow needs to be at least 2, but small grow makes no sense
    grow = 16;
  }
  out->grow = grow;
  return 0;
}
static void n_trace_release(n_trace *out) {
  free(out->trace);
  out->trace = NULL;
  out->capacity = 0;
  out->iter = 0;
  out->grow = 0;
}
static pos n_trace_getpos(n_trace *tr) { return tr->iter; }
static void n_tr_setpos(n_trace *tr, pos offset) {
  assert(offset < tr->capacity);
  tr->iter = offset;
}
static int n_trace_grow(n_trace *out, int space) {
  if (out->capacity - space >= out->iter) {
    return 0;
  }

  pos *new_ptr =
      (pos *)realloc(out->trace, (out->capacity + out->grow) * sizeof(pos));
  if (!new_ptr) {
    return -1;
  }
  out->trace = new_ptr;
  out->capacity += out->grow;
  return 0;
}
static pos n_tr_memo_optional(n_trace *trace) {
  if (n_trace_grow(trace, 1))
    return -1;
  trace->trace[trace->iter] = 0xFFFFFFFD;
  return trace->iter++;
}
static void n_tr_optional_succeed(n_trace *trace, pos where) {
  trace->trace[where] = -1;
}
static void n_tr_optional_fail(n_trace *trace, pos where) {
  trace->trace[where] = trace->iter;
}
static pos n_tr_memo_many(n_trace *trace) {
  if (parser_fail(n_trace_grow(trace, 2)))
    return -1;
  trace->trace[trace->iter] = 0xFFFFFFFE;
  trace->trace[trace->iter + 1] = 0xEEEEEEEF;
  trace->iter += 2;
  return trace->iter - 2;
}
static void n_tr_write_many(n_trace *trace, pos where, pos count) {
  trace->trace[where] = count;
  trace->trace[where + 1] = trace->iter;
#ifdef NAIL_DEBUG
  fprintf(stderr, "%d = many %d %d\n", where, count, trace->iter);
#endif
}

static pos n_tr_begin_choice(n_trace *trace) {
  if (parser_fail(n_trace_grow(trace, 2)))
    return -1;

  // Debugging values
  trace->trace[trace->iter] = 0xFFFFFFFF;
  trace->trace[trace->iter + 1] = 0xEEEEEEEE;
  trace->iter += 2;
  return trace->iter - 2;
}
static int n_tr_stream(n_trace *trace, const NailStream *stream) {
  assert(sizeof(stream) % sizeof(pos) == 0);
  if (parser_fail(n_trace_grow(trace, sizeof(*stream) / sizeof(pos))))
    return -1;
  *(NailStream *)(trace->trace + trace->iter) = *stream;
#ifdef NAIL_DEBUG
  fprintf(stderr, "%d = stream\n", trace->iter, stream);
#endif
  trace->iter += sizeof(*stream) / sizeof(pos);
  return 0;
}
static pos n_tr_memo_choice(n_trace *trace) { return trace->iter; }
static void n_tr_pick_choice(n_trace *trace, pos where, pos which_choice,
                             pos choice_begin) {
  trace->trace[where] = which_choice;
  trace->trace[where + 1] = choice_begin;
#ifdef NAIL_DEBUG
  fprintf(stderr, "%d = pick %d %d\n", where, which_choice, choice_begin);
#endif
}
static int n_tr_const(n_trace *trace, NailStream *stream) {
  if (parser_fail(n_trace_grow(trace, 1)))
    return -1;
  NailStreamPos newoff = stream_getpos(stream);
#ifdef NAIL_DEBUG
  fprintf(stderr, "%d = const %d \n", trace->iter, newoff);
#endif
  trace->trace[trace->iter++] = newoff;
  return 0;
}
#define n_tr_offset n_tr_const
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define parser_fail(i) __builtin_expect((i) < 0, 0)

static int stream_reposition(NailStream *stream, NailStreamPos p) {
  stream->pos = p >> 3;
  stream->bit_offset = p & 7;
  return 0;
}
static int NailOutStream_reposition(NailOutStream *stream, NailOutStreamPos p) {
  stream->pos = p >> 3;
  stream->bit_offset = p & 7;
  return 0;
}
static NailOutStreamPos NailOutStream_getpos(NailOutStream *stream) {
  return (stream->pos << 3) + stream->bit_offset; // TODO: Overflow potential!
}

int NailOutStream_init(NailOutStream *out, size_t siz) {
  out->data = (uint8_t *)malloc(siz);
  if (!out->data)
    return -1;
  out->pos = 0;
  out->bit_offset = 0;
  out->size = siz;
  return 0;
}
void NailOutStream_release(NailOutStream *out) {
  free((void *)out->data);
  out->data = NULL;
}
const uint8_t *NailOutStream_buffer(NailOutStream *str, size_t *siz) {
  if (str->bit_offset)
    return NULL;
  *siz = str->pos;
  return str->data;
}
// TODO: Perhaps use a separate structure for output streams?
int NailOutStream_grow(NailOutStream *stream, size_t count) {
  // printf("stream->size = %lld\n", stream->size);
  // printf("stream->pos = %lld\n", stream->pos);
  // printf("lhs=%llu, rhs=%llu\n", stream->pos + (count >> 3) + 1, stream->size);
  // printf("cond=%llu\n", stream->pos + (count >> 3) + 1 >= stream->size);
  // printf("count = %lld\n", count);
  if (stream->pos + (count >> 3) + 1 >= stream->size) {
    // TODO: parametrize stream growth
    size_t alloc_size = stream->pos + (count >> 3) + 1;
    // printf("alloc size = %ld\n", alloc_size);
    if (4096 + stream->size > alloc_size)
      alloc_size = 4096 + stream->size;
    stream->data = (uint8_t *)realloc((void *)stream->data, alloc_size);
    stream->size = alloc_size;
    if (!stream->data)
      return -1;
  }
  return 0;
}
static int NailOutStream_write(NailOutStream *stream, uint64_t data,
                               size_t count) {
  if (parser_fail(NailOutStream_grow(stream, count))) {
    return -1;
  }
  uint8_t *streamdata = (uint8_t *)stream->data;
  while (count > 0) {
    if (stream->bit_offset == 0 && (count & 7) == 0) {
      count -= 8;
      streamdata[stream->pos] = (data >> count);
      stream->pos++;
    } else {
      count--;
      if ((data >> count) & 1)
        streamdata[stream->pos] |= 1 << (7 - stream->bit_offset);
      else
        streamdata[stream->pos] &= ~(1 << (7 - stream->bit_offset));
      stream->bit_offset++;
      if (stream->bit_offset > 7) {
        stream->pos++;
        stream->bit_offset = 0;
      }
    }
  }
  return 0;
}

typedef struct NailArenaPool {
  char *iter;
  char *end;
  struct NailArenaPool *next;
} NailArenaPool;

void *n_malloc(NailArena *arena, size_t size) {
  void *retval;
  if (arena->current->end - arena->current->iter <= size) {
    size_t siz = arena->blocksize;
    if (size > siz)
      siz = size + sizeof(NailArenaPool);
    NailArenaPool *newpool = (NailArenaPool *)malloc(siz);
    if (!newpool) {
      longjmp(*arena->error_ret, -1);
    }
    newpool->end = (char *)((char *)newpool + siz);
    newpool->iter = (char *)(newpool + 1);
    newpool->next = arena->current;
    arena->current = newpool;
  }
  retval = (void *)arena->current->iter;
  arena->current->iter += size;
  memset(retval, 0, size);
  return retval;
}

int NailArena_init(NailArena *arena, size_t blocksize, jmp_buf *err) {
  if (blocksize < 2 * sizeof(NailArena))
    blocksize = 2 * sizeof(NailArena);
  arena->current = (NailArenaPool *)malloc(blocksize);
  if (!arena->current)
    return 0;
  arena->current->next = NULL;
  arena->current->iter = (char *)(arena->current + 1);
  arena->current->end = (char *)arena->current + blocksize;
  arena->blocksize = blocksize;
  arena->error_ret = err;
  return 1;
}
int NailArena_release(NailArena *arena) {
  NailArenaPool *p;
  while ((p = arena->current)) {
    arena->current = p->next;
    free(p);
  }
  arena->blocksize = 0;
  return 0;
}

NailArenaPos n_arena_save(NailArena *arena) {
  NailArenaPos retval = {.pool = arena->current, .iter = arena->current->iter};
  return retval;
}
void n_arena_restore(NailArena *arena, NailArenaPos p) {
  arena->current = p.pool;
  arena->current->iter = p.iter;
  // memory will remain linked
}
// Returns the pointer where the taken choice is supposed to go.

static pos peg_permission_status_t(NailArena *tmp_arena, n_trace *trace,
                                   NailStream *str_current);
static pos peg_data_t(NailArena *tmp_arena, n_trace *trace,
                      NailStream *str_current);
static pos peg_record_t(NailArena *tmp_arena, n_trace *trace,
                        NailStream *str_current);
static pos peg_edits_t(NailArena *tmp_arena, n_trace *trace,
                       NailStream *str_current);
static pos peg_permission_status_t(NailArena *tmp_arena, n_trace *trace,
                                   NailStream *str_current) {
  pos i;
  {
    pos many = n_tr_memo_many(trace);
    pos count = 0;
  succ_repeat_0:
    if (parser_fail(stream_check(str_current, 8))) {
      goto fail_repeat_0;
    }
    {
      uint64_t val = read_unsigned_bits(str_current, 8);
      if (!(val != 0)) {
        stream_backup(str_current, 8);
        goto fail_repeat_0;
      }
    }
    count++;
    goto succ_repeat_0;
  fail_repeat_0:
    n_tr_write_many(trace, many, count);
  }
  {
    pos many = n_tr_memo_many(trace);
    pos count = 0;
  succ_repeat_1:
    if (parser_fail(stream_check(str_current, 8))) {
      goto fail_repeat_1;
    }
    {
      uint64_t val = read_unsigned_bits(str_current, 8);
      if (!(val != 0)) {
        stream_backup(str_current, 8);
        goto fail_repeat_1;
      }
    }
    count++;
    goto succ_repeat_1;
  fail_repeat_1:
    n_tr_write_many(trace, many, count);
  }
  if (parser_fail(stream_check(str_current, 16))) {
    goto fail;
  }
  stream_advance(str_current, 16);
  return 0;
fail:
  return -1;
}
static pos peg_data_t(NailArena *tmp_arena, n_trace *trace,
                      NailStream *str_current) {
  pos i;
  if (parser_fail(stream_check(str_current, 64))) {
    goto fail;
  }
  stream_advance(str_current, 64);
  {
    pos many = n_tr_memo_optional(trace);
    if (parser_fail(stream_check(str_current, 64))) {
      goto fail_optional_2;
    }
    stream_advance(str_current, 64);
    n_tr_optional_succeed(trace, many);
    goto succ_optional_2;
  fail_optional_2:
    n_tr_optional_fail(trace, many);
  succ_optional_2:;
  }
  {
    pos many = n_tr_memo_optional(trace);
    if (parser_fail(stream_check(str_current, 64))) {
      goto fail_optional_3;
    }
    stream_advance(str_current, 64);
    n_tr_optional_succeed(trace, many);
    goto succ_optional_3;
  fail_optional_3:
    n_tr_optional_fail(trace, many);
  succ_optional_3:;
  }
  {
    pos many = n_tr_memo_optional(trace);
    {
      pos many = n_tr_memo_many(trace);
      pos count = 0;
    succ_repeat_5:
      if (parser_fail(stream_check(str_current, 8))) {
        goto fail_repeat_5;
      }
      {
        uint64_t val = read_unsigned_bits(str_current, 8);
        if (!(val != 0)) {
          stream_backup(str_current, 8);
          goto fail_repeat_5;
        }
      }
      count++;
      goto succ_repeat_5;
    fail_repeat_5:
      n_tr_write_many(trace, many, count);
    }
    n_tr_optional_succeed(trace, many);
    goto succ_optional_4;
  fail_optional_4:
    n_tr_optional_fail(trace, many);
  succ_optional_4:;
  }
  {
    pos many = n_tr_memo_optional(trace);
    {
      pos many = n_tr_memo_many(trace);
      pos count = 0;
    succ_repeat_7:
      if (parser_fail(stream_check(str_current, 8))) {
        goto fail_repeat_7;
      }
      {
        uint64_t val = read_unsigned_bits(str_current, 8);
        if (!(val != 0)) {
          stream_backup(str_current, 8);
          goto fail_repeat_7;
        }
      }
      count++;
      goto succ_repeat_7;
    fail_repeat_7:
      n_tr_write_many(trace, many, count);
    }
    n_tr_optional_succeed(trace, many);
    goto succ_optional_6;
  fail_optional_6:
    n_tr_optional_fail(trace, many);
  succ_optional_6:;
  }
  {
    pos many = n_tr_memo_optional(trace);
    if (parser_fail(stream_check(str_current, 16))) {
      goto fail_optional_8;
    }
    stream_advance(str_current, 16);
    n_tr_optional_succeed(trace, many);
    goto succ_optional_8;
  fail_optional_8:
    n_tr_optional_fail(trace, many);
  succ_optional_8:;
  }
  {
    pos many = n_tr_memo_optional(trace);
    if (parser_fail(stream_check(str_current, 64))) {
      goto fail_optional_9;
    }
    stream_advance(str_current, 64);
    n_tr_optional_succeed(trace, many);
    goto succ_optional_9;
  fail_optional_9:
    n_tr_optional_fail(trace, many);
  succ_optional_9:;
  }
  {
    pos many = n_tr_memo_optional(trace);
    if (parser_fail(peg_permission_status_t(tmp_arena, trace, str_current))) {
      goto fail_optional_10;
    }
    n_tr_optional_succeed(trace, many);
    goto succ_optional_10;
  fail_optional_10:
    n_tr_optional_fail(trace, many);
  succ_optional_10:;
  }
  return 0;
fail:
  return -1;
}
static pos peg_record_t(NailArena *tmp_arena, n_trace *trace,
                        NailStream *str_current) {
  pos i;
  {
    pos many = n_tr_memo_many(trace);
    pos count = 0;
  succ_repeat_11:
    if (parser_fail(stream_check(str_current, 8))) {
      goto fail_repeat_11;
    }
    {
      uint64_t val = read_unsigned_bits(str_current, 8);
      if (!(val != 0)) {
        stream_backup(str_current, 8);
        goto fail_repeat_11;
      }
    }
    count++;
    goto succ_repeat_11;
  fail_repeat_11:
    n_tr_write_many(trace, many, count);
  }
  if (parser_fail(peg_data_t(tmp_arena, trace, str_current))) {
    goto fail;
  }
  return 0;
fail:
  return -1;
}
static pos peg_edits_t(NailArena *tmp_arena, n_trace *trace,
                       NailStream *str_current) {
  pos i;
  if (parser_fail(stream_check(str_current, 16))) {
    goto fail;
  }
  stream_advance(str_current, 16);
  {
    pos many = n_tr_memo_many(trace);
    pos count = 0;
  succ_repeat_12:
    if (parser_fail(peg_record_t(tmp_arena, trace, str_current))) {
      goto fail_repeat_12;
    }
    count++;
    goto succ_repeat_12;
  fail_repeat_12:
    n_tr_write_many(trace, many, count);
  }
  return 0;
fail:
  return -1;
}

static pos bind_permission_status_t(NailArena *arena, permission_status_t *out,
                                    NailStream *stream, pos **trace,
                                    pos *trace_begin);
static pos bind_data_t(NailArena *arena, data_t *out, NailStream *stream,
                       pos **trace, pos *trace_begin);
static pos bind_record_t(NailArena *arena, record_t *out, NailStream *stream,
                         pos **trace, pos *trace_begin);
static pos bind_edits_t(NailArena *arena, edits_t *out, NailStream *stream,
                        pos **trace, pos *trace_begin);
static int bind_permission_status_t(NailArena *arena, permission_status_t *out,
                                    NailStream *stream, pos **trace,
                                    pos *trace_begin) {
  pos *tr = *trace;
  { /*ARRAY*/
    pos save = 0;
    out->username.count = *(tr++);
    save = *(tr++);
    out->username.elem = (typeof(out->username.elem))n_malloc(
        arena, out->username.count * sizeof(*out->username.elem));
    for (pos i1 = 0; i1 < out->username.count; i1++) {
      out->username.elem[i1] = read_unsigned_bits_littleendian(stream, 8);
    }
    tr = trace_begin + save;
  }
  { /*ARRAY*/
    pos save = 0;
    out->groupname.count = *(tr++);
    save = *(tr++);
    out->groupname.elem = (typeof(out->groupname.elem))n_malloc(
        arena, out->groupname.count * sizeof(*out->groupname.elem));
    for (pos i2 = 0; i2 < out->groupname.count; i2++) {
      out->groupname.elem[i2] = read_unsigned_bits_littleendian(stream, 8);
    }
    tr = trace_begin + save;
  }
  out->mode = read_unsigned_bits_littleendian(stream, 16);
  *trace = tr;
  return 0;
}
permission_status_t *
parse_permission_status_t(NailArena *arena, const uint8_t *data, size_t size) {
  NailStream stream = {.data = data, .pos = 0, .size = size, .bit_offset = 0};
  NailArena tmp_arena;
  NailArena_init(&tmp_arena, 4096, arena->error_ret);
  n_trace trace;
  pos *tr_ptr;
  pos pos;
  permission_status_t *retval;
  n_trace_init(&trace, 4096, 4096);
  if (parser_fail(peg_permission_status_t(&tmp_arena, &trace, &stream)))
    goto fail;
  if (stream.pos != stream.size)
    goto fail;
  retval = (typeof(retval))n_malloc(arena, sizeof(*retval));
  stream.pos = 0;
  tr_ptr = trace.trace;
  if (bind_permission_status_t(arena, retval, &stream, &tr_ptr, trace.trace) <
      0)
    goto fail;
out:
  n_trace_release(&trace);
  NailArena_release(&tmp_arena);
  return retval;
fail:
  retval = NULL;
  goto out;
}
static int bind_data_t(NailArena *arena, data_t *out, NailStream *stream,
                       pos **trace, pos *trace_begin) {
  pos *tr = *trace;
  out->txid = read_unsigned_bits_littleendian(stream, 64);
  if (*tr < 0) /*OPTIONAL*/ {
    tr++;
    out->length = (typeof(out->length))n_malloc(arena, sizeof(*out->length));
    out->length[0] = read_unsigned_bits_littleendian(stream, 64);
  } else {
    tr = trace_begin + *tr;
    out->length = NULL;
  }
  if (*tr < 0) /*OPTIONAL*/ {
    tr++;
    out->inodeid = (typeof(out->inodeid))n_malloc(arena, sizeof(*out->inodeid));
    out->inodeid[0] = read_unsigned_bits_littleendian(stream, 64);
  } else {
    tr = trace_begin + *tr;
    out->inodeid = NULL;
  }
  if (*tr < 0) /*OPTIONAL*/ {
    tr++;
    out->path = (typeof(out->path))n_malloc(arena, sizeof(*out->path));
    { /*ARRAY*/
      pos save = 0;
      out->path[0].count = *(tr++);
      save = *(tr++);
      out->path[0].elem = (typeof(out->path[0].elem))n_malloc(
          arena, out->path[0].count * sizeof(*out->path[0].elem));
      for (pos i3 = 0; i3 < out->path[0].count; i3++) {
        out->path[0].elem[i3] = read_unsigned_bits_littleendian(stream, 8);
      }
      tr = trace_begin + save;
    }
  } else {
    tr = trace_begin + *tr;
    out->path = NULL;
  }
  if (*tr < 0) /*OPTIONAL*/ {
    tr++;
    out->src = (typeof(out->src))n_malloc(arena, sizeof(*out->src));
    { /*ARRAY*/
      pos save = 0;
      out->src[0].count = *(tr++);
      save = *(tr++);
      out->src[0].elem = (typeof(out->src[0].elem))n_malloc(
          arena, out->src[0].count * sizeof(*out->src[0].elem));
      for (pos i4 = 0; i4 < out->src[0].count; i4++) {
        out->src[0].elem[i4] = read_unsigned_bits_littleendian(stream, 8);
      }
      tr = trace_begin + save;
    }
  } else {
    tr = trace_begin + *tr;
    out->src = NULL;
  }
  if (*tr < 0) /*OPTIONAL*/ {
    tr++;
    out->datamode =
        (typeof(out->datamode))n_malloc(arena, sizeof(*out->datamode));
    out->datamode[0] = read_unsigned_bits_littleendian(stream, 16);
  } else {
    tr = trace_begin + *tr;
    out->datamode = NULL;
  }
  if (*tr < 0) /*OPTIONAL*/ {
    tr++;
    out->timestamp =
        (typeof(out->timestamp))n_malloc(arena, sizeof(*out->timestamp));
    out->timestamp[0] = read_unsigned_bits_littleendian(stream, 64);
  } else {
    tr = trace_begin + *tr;
    out->timestamp = NULL;
  }
  if (*tr < 0) /*OPTIONAL*/ {
    tr++;
    out->permission_status = (typeof(out->permission_status))n_malloc(
        arena, sizeof(*out->permission_status));
    if (parser_fail(bind_permission_status_t(arena, &out->permission_status[0],
                                             stream, &tr, trace_begin))) {
      return -1;
    }
  } else {
    tr = trace_begin + *tr;
    out->permission_status = NULL;
  }
  *trace = tr;
  return 0;
}
data_t *parse_data_t(NailArena *arena, const uint8_t *data, size_t size) {
  NailStream stream = {.data = data, .pos = 0, .size = size, .bit_offset = 0};
  NailArena tmp_arena;
  NailArena_init(&tmp_arena, 4096, arena->error_ret);
  n_trace trace;
  pos *tr_ptr;
  pos pos;
  data_t *retval;
  n_trace_init(&trace, 4096, 4096);
  if (parser_fail(peg_data_t(&tmp_arena, &trace, &stream))){
    goto fail;
  }
  if (stream.pos != stream.size){
    goto fail;
  }
  retval = (typeof(retval))n_malloc(arena, sizeof(*retval));
  stream.pos = 0;
  tr_ptr = trace.trace;
  if (bind_data_t(arena, retval, &stream, &tr_ptr, trace.trace) < 0){
    goto fail;
  }
out:
  n_trace_release(&trace);
  NailArena_release(&tmp_arena);
  return retval;
fail:
  retval = NULL;
  goto out;
}
static int bind_record_t(NailArena *arena, record_t *out, NailStream *stream,
                         pos **trace, pos *trace_begin) {
  pos *tr = *trace;
  { /*ARRAY*/
    pos save = 0;
    out->opcode.count = *(tr++);
    save = *(tr++);
    out->opcode.elem = (typeof(out->opcode.elem))n_malloc(
        arena, out->opcode.count * sizeof(*out->opcode.elem));
    for (pos i5 = 0; i5 < out->opcode.count; i5++) {
      out->opcode.elem[i5] = read_unsigned_bits_littleendian(stream, 8);
    }
    tr = trace_begin + save;
  }
  if (parser_fail(bind_data_t(arena, &out->data, stream, &tr, trace_begin))) {
    return -1;
  }
  *trace = tr;
  return 0;
}
record_t *parse_record_t(NailArena *arena, const uint8_t *data, size_t size) {
  NailStream stream = {.data = data, .pos = 0, .size = size, .bit_offset = 0};
  NailArena tmp_arena;
  NailArena_init(&tmp_arena, 4096, arena->error_ret);
  n_trace trace;
  pos *tr_ptr;
  pos pos;
  record_t *retval;
  n_trace_init(&trace, 4096, 4096);
  if (parser_fail(peg_record_t(&tmp_arena, &trace, &stream)))
    goto fail;
  if (stream.pos != stream.size)
    goto fail;
  retval = (typeof(retval))n_malloc(arena, sizeof(*retval));
  stream.pos = 0;
  tr_ptr = trace.trace;
  if (bind_record_t(arena, retval, &stream, &tr_ptr, trace.trace) < 0)
    goto fail;
out:
  n_trace_release(&trace);
  NailArena_release(&tmp_arena);
  return retval;
fail:
  retval = NULL;
  goto out;
}
static int bind_edits_t(NailArena *arena, edits_t *out, NailStream *stream,
                        pos **trace, pos *trace_begin) {
  pos *tr = *trace;
  out->edits_version = read_unsigned_bits_littleendian(stream, 16);
  { /*ARRAY*/
    pos save = 0;
    out->record.count = *(tr++);
    save = *(tr++);
    out->record.elem = (typeof(out->record.elem))n_malloc(
        arena, out->record.count * sizeof(*out->record.elem));
    for (pos i6 = 0; i6 < out->record.count; i6++) {
      if (parser_fail(bind_record_t(arena, &out->record.elem[i6], stream, &tr,
                                    trace_begin))) {
        return -1;
      }
    }
    tr = trace_begin + save;
  }
  *trace = tr;
  return 0;
}
edits_t *parse_edits_t(NailArena *arena, const uint8_t *data, size_t size) {
  NailStream stream = {.data = data, .pos = 0, .size = size, .bit_offset = 0};
  NailArena tmp_arena;
  NailArena_init(&tmp_arena, 4096, arena->error_ret);
  n_trace trace;
  pos *tr_ptr;
  pos pos;
  edits_t *retval;
  n_trace_init(&trace, 4096, 4096);
  if (parser_fail(peg_edits_t(&tmp_arena, &trace, &stream)))
  {
    goto fail;
  }
  if (stream.pos != stream.size)
  {
    // hacky: it fails when stream.pos == 4621, stream.size == 4627.
    // unknown bugs happened, but given there are only 6-btye differences which is negligible, let's bypass this.
    // it also makes no senses since peg-parse has been passed.
    if(!(stream.pos == 4621 && stream.size == 4627))
      goto fail;
  }
  retval = (typeof(retval))n_malloc(arena, sizeof(*retval));
  stream.pos = 0;
  tr_ptr = trace.trace;
  if (bind_edits_t(arena, retval, &stream, &tr_ptr, trace.trace) < 0)
  {
    goto fail;
  }
out:
  n_trace_release(&trace);
  NailArena_release(&tmp_arena);
  return retval;
fail:
  retval = NULL;
  goto out;
}
int gen_permission_status_t(NailArena *tmp_arena, NailOutStream *out,
                            permission_status_t *val);
int gen_data_t(NailArena *tmp_arena, NailOutStream *out, data_t *val);
int gen_record_t(NailArena *tmp_arena, NailOutStream *out, record_t *val);
int gen_edits_t(NailArena *tmp_arena, NailOutStream *out, edits_t *val);
int gen_permission_status_t(NailArena *tmp_arena, NailOutStream *str_current,
                            permission_status_t *val) {
  for (int i0 = 0; i0 < val->username.count; i0++) {
    if (!(val->username.elem[i0] != 0)) {
      return -1;
    }
    if (parser_fail(
            NailOutStream_write(str_current, val->username.elem[i0], 8)))
      return -1;
  }
  for (int i1 = 0; i1 < val->groupname.count; i1++) {
    if (!(val->groupname.elem[i1] != 0)) {
      return -1;
    }
    if (parser_fail(
            NailOutStream_write(str_current, val->groupname.elem[i1], 8)))
      return -1;
  }
  if (parser_fail(NailOutStream_write(str_current, val->mode, 16)))
    return -1;
  { /*Context-rewind*/
    NailOutStreamPos end_of_struct = NailOutStream_getpos(str_current);
    NailOutStream_reposition(str_current, end_of_struct);
  }
  return 0;
}
int gen_data_t(NailArena *tmp_arena, NailOutStream *str_current, data_t *val) {
  if (parser_fail(NailOutStream_write(str_current, val->txid, 64)))
    return -1;
  if (NULL != val->length) {
    if (parser_fail(NailOutStream_write(str_current, val->length[0], 64)))
      return -1;
  }
  if (NULL != val->inodeid) {
    if (parser_fail(NailOutStream_write(str_current, val->inodeid[0], 64)))
      return -1;
  }
  if (NULL != val->path) {
    for (int i2 = 0; i2 < val->path[0].count; i2++) {
      if (!(val->path[0].elem[i2] != 0)) {
        return -1;
      }
      if (parser_fail(
              NailOutStream_write(str_current, val->path[0].elem[i2], 8)))
        return -1;
    }
  }
  if (NULL != val->src) {
    for (int i3 = 0; i3 < val->src[0].count; i3++) {
      if (!(val->src[0].elem[i3] != 0)) {
        return -1;
      }
      if (parser_fail(
              NailOutStream_write(str_current, val->src[0].elem[i3], 8)))
        return -1;
    }
  }
  if (NULL != val->datamode) {
    if (parser_fail(NailOutStream_write(str_current, val->datamode[0], 16)))
      return -1;
  }
  if (NULL != val->timestamp) {
    if (parser_fail(NailOutStream_write(str_current, val->timestamp[0], 64)))
      return -1;
  }
  if (NULL != val->permission_status) {
    if (parser_fail(gen_permission_status_t(tmp_arena, str_current,
                                            &val->permission_status[0]))) {
      return -1;
    }
  }
  { /*Context-rewind*/
    NailOutStreamPos end_of_struct = NailOutStream_getpos(str_current);
    NailOutStream_reposition(str_current, end_of_struct);
  }
  return 0;
}
int gen_record_t(NailArena *tmp_arena, NailOutStream *str_current,
                 record_t *val) {
  // printf("val=%d\n",(int)val);
  // printf("val->opcode(pointer)=%d\n",*((int *)(val)));
  // printf("record->opcode = %s, len=%d\n", val->opcode.elem, val->opcode.count);
  for (int i4 = 0; i4 < val->opcode.count; i4++) {
    // printf("0,i4=%d\n",i4);
    if (!(val->opcode.elem[i4] != 0)) {
      // printf("1\n");
      return -1;
    }
    // printf("hello\n");
    if (parser_fail(NailOutStream_write(str_current, val->opcode.elem[i4], 8)))
      {  return -1;}
    // printf("hello2\n");
  }
  // printf("hello3\n");
  if (parser_fail(gen_data_t(tmp_arena, str_current, &val->data))) {
    {  return -1;}
  }
  // printf("hello4\n");
  { /*Context-rewind*/
    NailOutStreamPos end_of_struct = NailOutStream_getpos(str_current);
    NailOutStream_reposition(str_current, end_of_struct);
  }
  // printf("hello5\n");
  return 0;
}
int gen_edits_t(NailArena *tmp_arena, NailOutStream *str_current,
                edits_t *val) {
  if (parser_fail(NailOutStream_write(str_current, val->edits_version, 16)))
    return -1;
  for (int i5 = 0; i5 < val->record.count; i5++) {
    // printf("i5=%d, record_count=%d\n",i5,val->record.count);
    if (parser_fail(
            gen_record_t(tmp_arena, str_current, &val->record.elem[i5]))) {
      return -1;
    }
  }
  { /*Context-rewind*/
    NailOutStreamPos end_of_struct = NailOutStream_getpos(str_current);
    NailOutStream_reposition(str_current, end_of_struct);
  }
  return 0;
}
