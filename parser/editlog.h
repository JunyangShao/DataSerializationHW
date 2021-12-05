#include <assert.h>
#include <stdint.h>
#include <string.h>
enum N_types { _NAIL_NULL };
typedef struct permission_status_t permission_status_t;
typedef struct data_t data_t;
typedef struct record_t record_t;
typedef struct edits_t edits_t;
struct permission_status_t {
  struct {
    int8_t *elem;
    size_t count;
  } username;
  struct {
    int8_t *elem;
    size_t count;
  } groupname;
  uint16_t mode;
};
struct data_t {
  uint64_t txid;
  uint64_t *length;
  uint64_t *inodeid;
  struct {
    int8_t *elem;
    size_t count;
  } * path;
  struct {
    int8_t *elem;
    size_t count;
  } * src;
  uint64_t *timestamp;
  permission_status_t *permission_status;
};
struct record_t {
  struct {
    int8_t *elem;
    size_t count;
  } opcode;
  data_t data;
};
struct edits_t {
  int16_t edits_version;
  struct {
    record_t *elem;
    size_t count;
  } record;
};

#include <setjmp.h>
struct NailArenaPool;
struct NailArena;
struct NailArenaPos;
typedef struct NailArena NailArena;
typedef struct NailArenaPos NailArenaPos;
NailArenaPos n_arena_save(NailArena *arena);
void n_arena_restore(NailArena *arena, NailArenaPos p);

struct NailArena {
  struct NailArenaPool *current;
  size_t blocksize;
  jmp_buf *error_ret; // XXX: Leaks memory on OOM. Keep a linked list of
                      // erroring arenas?
};
struct NailArenaPos {
  struct NailArenaPool *pool;
  char *iter;
};
extern int NailArena_init(NailArena *arena, size_t blocksize,
                          jmp_buf *error_return);
extern int NailArena_release(NailArena *arena);
extern void *n_malloc(NailArena *arena, size_t size);
struct NailStream {
  const uint8_t *data;
  size_t size;
  size_t pos;
  signed char bit_offset;
};

struct NailOutStream {
  uint8_t *data;
  size_t size;
  size_t pos;
  signed char bit_offset;
};
typedef struct NailStream NailStream;
typedef struct NailOutStream NailOutStream;
typedef size_t NailStreamPos;
typedef size_t NailOutStreamPos;
static NailStream *NailStream_alloc(NailArena *arena) {
  return (NailStream *)n_malloc(arena, sizeof(NailStream));
}
static NailOutStream *NailOutStream_alloc(NailArena *arena) {
  return (NailOutStream *)n_malloc(arena, sizeof(NailOutStream));
}
extern int NailOutStream_init(NailOutStream *str, size_t siz);
extern void NailOutStream_release(NailOutStream *str);
const uint8_t *NailOutStream_buffer(NailOutStream *str, size_t *siz);
extern int NailOutStream_grow(NailOutStream *stream, size_t count);

#define n_fail(i) __builtin_expect(i, 0)
permission_status_t *
parse_permission_status_t(NailArena *arena, const uint8_t *data, size_t size);
data_t *parse_data_t(NailArena *arena, const uint8_t *data, size_t size);
record_t *parse_record_t(NailArena *arena, const uint8_t *data, size_t size);
edits_t *parse_edits_t(NailArena *arena, const uint8_t *data, size_t size);

int gen_permission_status_t(NailArena *tmp_arena, NailOutStream *out,
                            permission_status_t *val);
int gen_data_t(NailArena *tmp_arena, NailOutStream *out, data_t *val);
int gen_record_t(NailArena *tmp_arena, NailOutStream *out, record_t *val);
int gen_edits_t(NailArena *tmp_arena, NailOutStream *out, edits_t *val);
