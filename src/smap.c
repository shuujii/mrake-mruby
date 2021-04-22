/*
 * Map implementation `mrb_sym` to `mrb_value`
 *
 * This is a memory-saving oriented map. Public functions allow the value of
 * `mrb_smap *` to be `NULL`, where` NULL` represents an empty map. Memory
 * allocation is done with `mrb_smap_set` as needed.
 */

#include <string.h>
#include <mruby.h>
#include <mruby/smap.h>

/*
 * Table structure layout:
 *                                         mrb_smap* --.
 *                                                      \
 *           +---------+---------+---+-------+-------+---o--------+--------+
 *   Type    |mrb_value|mrb_value|...|mrb_sym|mrb_sym|...|uint16_t|uint16_t|
 *           +---------+---------+---+-------+-------+---+--------+--------+
 *   Content |  val 1  |  val 2  |...| sym 1 | sym 2 |...|  size  | capa_1 |
 *           +---------+---------+---+-------+-------+---+--------+--------+
 *
 *   - Only `val` and only `sym` are contiguous to eliminate structure padding.
 *   - `val` is placed at the beginning to align it at 8-byte boundary.
 *   - `capa_1` means "capacity - 1".
 */
struct mrb_smap {
  uint16_t size;
  uint16_t capa_1;
};

typedef struct mrb_smap_iter {
  mrb_sym *syms;
  mrb_value *vals;
  uint16_t idx;
  uint16_t mask;
} mrb_smap_iter;

#define SMAP_INIT_CAPA 1
#define SMAP_MAX_CAPA (U32(UINT16_MAX) + 1)
#define SMAP_MAX_SIZE UINT16_MAX
#define SMAP_EMPTY_SYM UINT32_MAX
#define SMAP_DELETED_SYM (SMAP_EMPTY_SYM - 1)

#define U16(v) ((uint16_t)(v))
#define U32(v) ((uint32_t)(v))

#define sym_hash_code(sym) U32((sym) ^ ((sym) << 2) ^ ((sym) >> 2))
#define smap_syms(t) ((mrb_sym *)((char *)(t) - sizeof(mrb_sym) * smap_capa(t)))
#define smap_vals(t) ((mrb_value *)((char *)(t) - smap_offset_for(smap_capa(t))))

#define smap_each_by_hash_code(t, hash_code, it_var, code) do {               \
  uint32_t capa__ = smap_capa(t);                                             \
  mrb_smap_iter it_var[1];                                                    \
  smap_it_init_by_hash_code(it_var, t, hash_code);                            \
  for (; capa__; smap_it_next_by_hash_code(it_var), --capa__) {               \
    code;                                                                     \
  }                                                                           \
} while (0)

#define smap_unsafe_full_each(t, it_var, code) do {                           \
  mrb_smap_iter it_var[1];                                                    \
  smap_it_init(it_var, t);                                                    \
  for (; TRUE; smap_it_next(it_var)) {                                        \
    code;                                                                     \
  }                                                                           \
} while (0)

static size_t smap_offset_for(uint32_t capa);
static uint32_t smap_capa(const mrb_smap *t);

static uint16_t
smap_it_idx_for(const mrb_smap_iter *it, uint32_t v)
{
  return v & it->mask;
}

static void
smap_it_init(mrb_smap_iter *it, mrb_smap *t)
{
  it->syms = smap_syms(t);
  it->vals = smap_vals(t);
  it->idx = 0;
}

static void
smap_it_init_by_hash_code(mrb_smap_iter *it, mrb_smap *t, uint32_t hash_code)
{
  it->mask = t->capa_1;
  it->syms = smap_syms(t);
  it->vals = smap_vals(t);
  it->idx = smap_it_idx_for(it, hash_code);
}

static void
smap_it_next(mrb_smap_iter *it)
{
  ++it->idx;
}

static void
smap_it_next_by_hash_code(mrb_smap_iter *it)
{
  it->idx = smap_it_idx_for(it, it->idx + 1);
}

static mrb_sym
smap_it_sym(const mrb_smap_iter *it)
{
  return it->syms[it->idx];
}

static mrb_value
smap_it_val(const mrb_smap_iter *it)
{
  return it->vals[it->idx];
}

static mrb_bool
smap_it_empty_p(const mrb_smap_iter *it)
{
  return smap_it_sym(it) == SMAP_EMPTY_SYM;
}

static mrb_bool
smap_it_active_p(const mrb_smap_iter *it)
{
  return smap_it_sym(it) < SMAP_DELETED_SYM;
}

static void
smap_it_set_val(mrb_smap_iter *it, mrb_value val)
{
  it->vals[it->idx] = val;
}

static void
smap_it_set(mrb_smap_iter *it, mrb_sym sym, mrb_value val)
{
  it->syms[it->idx] = sym;
  it->vals[it->idx] = val;
}

static void
smap_it_delete(mrb_smap_iter *it)
{
  it->syms[it->idx] = SMAP_DELETED_SYM;
}

static size_t
smap_offset_for(uint32_t capa)
{
  return (sizeof(mrb_value) + sizeof(mrb_sym)) * capa;
}

static size_t
smap_memsize_for(uint32_t capa)
{
  return smap_offset_for(capa) + sizeof(uint16_t) * 2;
}

static uint16_t
smap_size(const mrb_smap *t)
{
  return t->size;
}

static void
smap_set_size(mrb_smap *t, uint16_t size)
{
  t->size = size;
}

static uint32_t
smap_capa(const mrb_smap *t)
{
  return U32(t->capa_1) + 1;
}

static void
smap_set_capa(mrb_smap *t, uint32_t capa)
{
  t->capa_1 = U16(capa - 1);
}

static mrb_smap *
smap_new(mrb_state *mrb, uint32_t capa)
{
  size_t memsize = smap_memsize_for(capa), offset = smap_offset_for(capa);
  mrb_smap *t = (mrb_smap *)((char *)mrb_malloc(mrb, memsize) + offset);
  smap_set_size(t, 0);
  smap_set_capa(t, capa);
  memset(smap_syms(t), 0xff, sizeof(mrb_sym) * capa);
  return t;
}

static void
smap_free(mrb_state *mrb, mrb_smap *t)
{
  mrb_free(mrb, (char *)(t) - smap_offset_for(smap_capa(t)));
}

static void
smap_expand(mrb_state *mrb, mrb_smap **tp)
{
  mrb_smap *t = *tp;
  uint32_t capa = smap_capa(t), new_capa = capa * 2;
  mrb_assert(capa < SMAP_MAX_CAPA);
  mrb_assert(capa == smap_size(t));
  mrb_smap *new_t = smap_new(mrb, new_capa);
  smap_set_size(new_t, smap_size(t));
  smap_set_capa(new_t, new_capa);
  smap_unsafe_full_each(t, it, {
    mrb_assert(smap_it_active_p(it));
    mrb_sym sym = smap_it_sym(it);
    smap_each_by_hash_code(new_t, sym_hash_code(sym), new_it, {
      if (!smap_it_empty_p(new_it)) continue;
      smap_it_set(new_it, sym, smap_it_val(it));
      break;
    });
    if (!--capa) break;
  });
  smap_free(mrb, t);
  *tp = new_t;
}

/* Get the memory size of the map. */
size_t
mrb_smap_memsize(const mrb_smap *t)
{
  return t ? smap_memsize_for(smap_capa(t)) : 0;
}

/* Get the size of the map. */
uint16_t
mrb_smap_size(const mrb_smap *t)
{
  return t ? t->size : 0;
}

/* Set the value for the symbol in the map. Note that `*tp` may change. */
void
mrb_smap_set(mrb_state *mrb, mrb_smap **tp, mrb_sym sym, mrb_value val)
{
  if (!*tp) *tp = smap_new(mrb, SMAP_INIT_CAPA);
  uint16_t size = smap_size(*tp);
  uint32_t hash_code = sym_hash_code(sym);
  smap_each_by_hash_code(*tp, hash_code, it, {
    if (smap_it_sym(it) == sym) {
      smap_it_set_val(it, val);
      return;
    }
    if (!smap_it_active_p(it)) {
      if (size == SMAP_MAX_SIZE) mrb_raise(mrb, E_ARGUMENT_ERROR, "map too big");
      smap_it_set(it, sym, val);
      smap_set_size(*tp, ++size);
      return;
    }
  });
  smap_expand(mrb, tp);
  smap_each_by_hash_code(*tp, hash_code, it, {
    if (!smap_it_empty_p(it)) continue;
    smap_it_set(it, sym, val);
    smap_set_size(*tp, ++size);
    return;
  });
  mrb_assert("not reached");
}

/* Get a value for a symbol from the map. */
mrb_bool
mrb_smap_get(const mrb_smap *t, mrb_sym sym, mrb_value *valp)
{
  if (!t) return FALSE;
  smap_each_by_hash_code((mrb_smap*)t, sym_hash_code(sym), it, {
    if (smap_it_empty_p(it)) return FALSE;
    if (smap_it_sym(it) != sym) continue;
    if (valp) *valp = smap_it_val(it);
    return TRUE;
  });
  return FALSE;
}

/* Deletes the value for the symbol from the map. */
mrb_bool
mrb_smap_delete(mrb_state *mrb, mrb_smap **tp, mrb_sym sym, mrb_value *valp)
{
  if (!*tp) return FALSE;
  smap_each_by_hash_code(*tp, sym_hash_code(sym), it, {
    if (smap_it_empty_p(it)) return FALSE;
    if (smap_it_sym(it) != sym) continue;
    if (valp) *valp = smap_it_val(it);
    smap_it_delete(it);
    smap_set_size(*tp, smap_size(*tp) - 1);
    return TRUE;
  });
  return FALSE;
}

/* Iterates over the map. */
void
mrb_smap_each(mrb_state *mrb, mrb_smap *t, mrb_smap_each_func *func, void *data)
{
  uint16_t size;
  if (!t || (size = smap_size(t)) == 0) return;
  smap_unsafe_full_each(t, it, {
    if (!smap_it_active_p(it)) continue;
    if (func(mrb, smap_it_sym(it), smap_it_val(it), data) != 0) return;
    if (!--size) return;
  });
}

/* Copy the map. */
mrb_smap *
mrb_smap_copy(mrb_state *mrb, const mrb_smap *t)
{
  if (!t) return NULL;
  uint32_t capa = smap_capa(t);
  size_t memsize = smap_memsize_for(capa);
  void *p = mrb_malloc(mrb, memsize);
  memcpy(p, smap_vals(t), memsize);
  return (mrb_smap *)((char *)p + smap_offset_for(capa));
}

/* Free the map (NULL safe). */
void
mrb_smap_free(mrb_state *mrb, mrb_smap *t)
{
  if (t) smap_free(mrb, t);
}
