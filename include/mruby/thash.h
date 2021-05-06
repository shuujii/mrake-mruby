#ifndef MRUBY_THASH_H
#define MRUBY_THASH_H

/*
 * Tiny hash table based map/set template.
 *
 * This provides a memory-saving oriented map/set. Public functions allow
 * the value of `NAME *` (aka `mrb_thash *`) to be `NULL`, where` NULL`
 * represents an empty map/set. Memory allocation is done with `NAME_add` or
 * `NAME_set` as needed.
 */

#include <string.h>

#define MRB_TMAP_DECLARE(name_, K, V)                                         \
  _MRB_THASH_DECLARE(name_, K, V)                                             \
  void name_##_set(mrb_state *mrb, name_ **tp, K key, V val);
#define MRB_TSET_DECLARE(name_, K)                                            \
  _MRB_THASH_DECLARE(name_, K, mrb_tset_pseudo_value)
#define MRB_TMAP_DEFINE(name_, K, V, empty_key_, deleted_key_,                \
                        active_key_p_func_, hash_func_, equal_p_func_)        \
  _MRB_THASH_DEFINE(name_, K, V, empty_key_, deleted_key_,                    \
                    active_key_p_func_, hash_func_, equal_p_func_)            \
  /* Set the value for the key to the map. Note that `*tp` may change. */     \
  void                                                                        \
  name_##_set(mrb_state *mrb, name_ **tp, K key, V val)                       \
  {                                                                           \
    name_##_iter it = name_##_add(mrb, tp, key);                              \
    name_##_it_set_value(it, val);                                            \
  }
#define MRB_TSET_DEFINE(name_, K, empty_key_, deleted_key_,                   \
                        active_key_p_func_, hash_func_, equal_p_func_)        \
  _MRB_THASH_DEFINE(name_, K, mrb_tset_pseudo_value, empty_key_, deleted_key_,\
                    active_key_p_func_, hash_func_, equal_p_func_)
#define MRB_TMAP_INIT(name_, K, V, empty_key_, deleted_key_,                  \
                      active_key_p_func_, hash_func_, equal_p_func_)          \
  MRB_TMAP_DECLARE(name_, K, V)                                               \
  MRB_TMAP_DEFINE(name_, K, V, empty_key_, deleted_key_,                      \
                  active_key_p_func_, hash_func_, equal_p_func_)
#define MRB_TSET_INIT(name_, K, empty_key_, deleted_key_,                     \
                      active_key_p_func_, hash_func_, equal_p_func_)          \
  MRB_TSET_DECLARE(name_, K)                                                  \
  MRB_TSET_DEFINE(name_, K, empty_key_, deleted_key_,                         \
                  active_key_p_func_, hash_func_, equal_p_func_)

#define _MRB_THASH_DECLARE(name_, K, V)                                       \
  /*                                                                          \
   * Table structure layout:                                                  \
   *                                                                          \
   *                        NAME* (mrb_thash*) --.                            \
   *                                              \                           \
   *           +-------+-------+---+-----+-----+---o--------+--------+        \
   *   Type    |   V   |   V   |...|  K  |  K  |...|uint16_t|uint16_t|        \
   *           +-------+-------+---+-----+-----+---+--------+--------+        \
   *   Content |value 1|value 2|...|key 1|key 2|...|  size  | capa_1 |        \
   *           +-------+-------+---+-----+-----+---+--------+--------+        \
   *                                                                          \
   *   - Only `value` and only `key` are contiguous to eliminate structure    \
   *     padding.                                                             \
   *   - `value` is placed at the beginning to align it at 8-byte boundary.   \
   *   - `capa_1` means "capacity - 1".                                       \
   */                                                                         \
  typedef struct name_ {                                                      \
    uint16_t size;                                                            \
    uint16_t capa_1;                                                          \
  } name_;                                                                    \
                                                                              \
  typedef struct name_##_iter {                                               \
    name_ *t;                                                                 \
    uint16_t idx;                                                             \
  } name_##_iter;                                                             \
                                                                              \
  typedef int (name_##_each_func)(mrb_state *, name_##_iter, void *);         \
                                                                              \
  size_t name_##_memsize(const name_ *t);                                     \
  uint16_t name_##_size(const name_ *t);                                      \
  mrb_bool name_##_include_p(const name_ *t, K key);                          \
  name_##_iter name_##_add(mrb_state *mrb, name_ **tp, K key);                \
  name_##_iter name_##_get(const name_ *t, K key);                            \
  void name_##_delete(mrb_state *mrb, name_ **tp, K key);                     \
  void name_##_delete_by_it(mrb_state *mrb, name_ **tp, name_##_iter it);     \
  void name_##_each(mrb_state *mrb, name_ *t, name_##_each_func *f, void *d); \
  name_ *name_##_copy(mrb_state *mrb, const name_ *t);                        \
  void name_##_free(mrb_state *mrb, name_ *t);                                \
                                                                              \
  MRB_INLINE mrb_unused uint16_t                                              \
  _##name_##_size(const name_ *t)                                             \
  {                                                                           \
    return _mrb_thash_size(t);                                                \
  }                                                                           \
                                                                              \
  MRB_INLINE mrb_unused uint32_t                                              \
  _##name_##_capa(const name_ *t)                                             \
  {                                                                           \
    return _mrb_thash_capa(t);                                                \
  }                                                                           \
                                                                              \
  MRB_INLINE mrb_unused size_t                                                \
  _##name_##_offset_for(uint32_t capa)                                        \
  {                                                                           \
    return (sizeof(V) + sizeof(K)) * capa;                                    \
  }                                                                           \
                                                                              \
  MRB_INLINE mrb_unused K *                                                   \
  _##name_##_keys(name_ *t)                                                   \
  {                                                                           \
    return (K *)((char *)(t) - sizeof(K) * _##name_##_capa(t));               \
  }                                                                           \
                                                                              \
  MRB_INLINE mrb_unused V *                                                   \
  _##name_##_values(name_ *t)                                                 \
  {                                                                           \
    return (V *)((char *)(t) - _##name_##_offset_for(_##name_##_capa(t)));    \
  }                                                                           \
                                                                              \
  MRB_INLINE mrb_unused name_##_iter                                          \
  _##name_##_it(name_ *t, uint16_t idx)                                       \
  {                                                                           \
    name_##_iter it;                                                          \
    it.t = t;                                                                 \
    it.idx = idx;                                                             \
    return it;                                                                \
  }                                                                           \
                                                                              \
  MRB_INLINE mrb_unused name_##_iter                                          \
  _##name_##_it_null(void)                                                    \
  {                                                                           \
    return _##name_##_it(NULL, MRB_THASH_MAX_SIZE);                           \
  }                                                                           \
                                                                              \
  /* Return true if the iterator is the null iterator. */                     \
  MRB_INLINE mrb_unused mrb_bool                                              \
  name_##_it_null_p(const name_##_iter it)                                    \
  {                                                                           \
    return it.t == NULL;                                                      \
  }                                                                           \
                                                                              \
  /* Get the key for the iterator. */                                         \
  MRB_INLINE mrb_unused K                                                     \
  name_##_it_key(const name_##_iter it)                                       \
  {                                                                           \
    return _##name_##_keys(it.t)[it.idx];                                     \
  }                                                                           \
                                                                              \
  /* Get the value for the iterator. */                                       \
  MRB_INLINE mrb_unused V                                                     \
  name_##_it_value(const name_##_iter it)                                     \
  {                                                                           \
    return _##name_##_values(it.t)[it.idx];                                   \
  }                                                                           \
                                                                              \
  /* Set the value for the iterator. */                                       \
  MRB_INLINE mrb_unused void                                                  \
  name_##_it_set_value(name_##_iter it, V val)                                \
  {                                                                           \
    _##name_##_values(it.t)[it.idx] = val;                                    \
  }

#define _MRB_THASH_DEFINE(name_, K, V, empty_key_, deleted_key_,              \
                          active_key_p_func_, hash_func_, equal_p_func_)      \
  static void                                                                 \
  _##name_##_set_size(name_ *t, uint16_t size)                                \
  {                                                                           \
    _mrb_thash_set_size(t, size);                                             \
  }                                                                           \
                                                                              \
  static void                                                                 \
  _##name_##_set_capa(name_ *t, uint32_t capa)                                \
  {                                                                           \
    _mrb_thash_set_capa(t, capa);                                             \
  }                                                                           \
                                                                              \
  static void                                                                 \
  _##name_##_it_set_key(name_##_iter it, K key)                               \
  {                                                                           \
    _##name_##_keys(it.t)[it.idx] = key;                                      \
  }                                                                           \
                                                                              \
  static void                                                                 \
  _##name_##_it_delete(name_##_iter it)                                       \
  {                                                                           \
    _##name_##_it_set_key(it, deleted_key_);                                  \
  }                                                                           \
                                                                              \
  static size_t                                                               \
  _##name_##_memsize_for(uint32_t capa)                                       \
  {                                                                           \
    return _##name_##_offset_for(capa) + sizeof(name_);                       \
  }                                                                           \
                                                                              \
  static name_ *                                                              \
  _##name_##_new(mrb_state *mrb, uint32_t capa)                               \
  {                                                                           \
    size_t memsize = _##name_##_memsize_for(capa);                            \
    size_t offset = _##name_##_offset_for(capa);                              \
    name_ *t = (name_ *)((char *)mrb_malloc(mrb, memsize) + offset);          \
    _##name_##_set_size(t, 0);                                                \
    _##name_##_set_capa(t, capa);                                             \
    for (K *kp = _##name_##_keys(t); kp != (K *)t; ++kp) *kp = empty_key_;    \
    return t;                                                                 \
  }                                                                           \
                                                                              \
  static void                                                                 \
  _##name_##_free(mrb_state *mrb, name_ *t)                                   \
  {                                                                           \
    mrb_free(mrb, (char *)(t) - _##name_##_offset_for(_##name_##_capa(t)));   \
  }                                                                           \
                                                                              \
  static void                                                                 \
  _##name_##_expand(mrb_state *mrb, name_ **tp)                               \
  {                                                                           \
    name_ *t = *tp;                                                           \
    uint16_t size = _##name_##_size(t);                                       \
    uint32_t new_capa = _##name_##_capa(t) * 2;                               \
    mrb_assert(_##name_##_capa(t) < MRB_THASH_MAX_CAPA);                      \
    mrb_assert(_##name_##_capa(t) == size);                                   \
    name_ *new_t = _##name_##_new(mrb, new_capa);                             \
    _##name_##_set_size(new_t, size);                                         \
    _##name_##_set_capa(new_t, new_capa);                                     \
    K *keys = _##name_##_keys(t), *new_keys = _##name_##_keys(new_t);         \
    V *vals = _##name_##_values(t), *new_vals = _##name_##_values(new_t);     \
    for (uint16_t idx = 0; idx < size; ++idx) {                               \
      mrb_assert(active_key_p_func_(keys[idx]));                              \
      _mrb_thash_probe(new_t, hash_func_(keys[idx]), new_idx, {               \
        if (new_keys[new_idx] != empty_key_) continue;                        \
        new_keys[new_idx] = keys[idx];                                        \
        new_vals[new_idx] = vals[idx];                                        \
        break;                                                                \
      });                                                                     \
    }                                                                         \
    _##name_##_free(mrb, t);                                                  \
    *tp = new_t;                                                              \
  }                                                                           \
                                                                              \
  /* Get the memory size of the map/set. */                                   \
  size_t                                                                      \
  name_##_memsize(const name_ *t)                                             \
  {                                                                           \
    return t ? _##name_##_memsize_for(_##name_##_capa(t)) : 0;                \
  }                                                                           \
                                                                              \
  /* Get the size of the map/set. */                                          \
  uint16_t                                                                    \
  name_##_size(const name_ *t)                                                \
  {                                                                           \
    return t ? _##name_##_size(t) : 0;                                        \
  }                                                                           \
                                                                              \
  /* Return true if the map/set includes the key */                           \
  mrb_bool                                                                    \
  name_##_include_p(const name_ *t, K key)                                    \
  {                                                                           \
    return !name_##_it_null_p(name_##_get(t, key));                           \
  }                                                                           \
                                                                              \
  /*                                                                          \
   * Adds the key to the map/set if it is not already present. Return the     \
   * iterator to which the key is mapped.                                     \
   */                                                                         \
  name_##_iter                                                                \
  name_##_add(mrb_state *mrb, name_ **tp, K key)                              \
  {                                                                           \
    if (!*tp) *tp = _##name_##_new(mrb, MRB_THASH_INIT_CAPA);                 \
    uint16_t size = _##name_##_size(*tp);                                     \
    uint32_t hash_code = hash_func_(key);                                     \
    K *keys = _##name_##_keys(*tp);                                           \
    _mrb_thash_probe(*tp, hash_code, idx, {                                   \
      if (active_key_p_func_(keys[idx])) {                                    \
        if (equal_p_func_(key, keys[idx])) return _##name_##_it(*tp, idx);    \
      } else if (size == MRB_THASH_MAX_SIZE) {                                \
        mrb_raise(mrb, E_ARGUMENT_ERROR, "map too big");                      \
      } else {                                                                \
        keys[idx] = key;                                                      \
        _##name_##_set_size(*tp, ++size);                                     \
        return _##name_##_it(*tp, idx);                                       \
       }                                                                      \
    });                                                                       \
    _##name_##_expand(mrb, tp);                                               \
    keys = _##name_##_keys(*tp);                                              \
    _mrb_thash_probe(*tp, hash_code, idx, {                                   \
      if (keys[idx] != empty_key_) continue;                                  \
      keys[idx] = key;                                                        \
      _##name_##_set_size(*tp, ++size);                                       \
      return _##name_##_it(*tp, idx);                                         \
    });                                                                       \
    mrb_assert("not reached");                                                \
    return _##name_##_it_null();                                              \
  }                                                                           \
                                                                              \
  /*                                                                          \
   * Return the iterator to which the key is mapped, or null iterator if      \
   * this map/set includes no mapping for the key.                            \
   */                                                                         \
  name_##_iter                                                                \
  name_##_get(const name_ *t, K key)                                          \
  {                                                                           \
    if (!t) return _##name_##_it_null();                                      \
    const K *keys = _##name_##_keys((name_ *)t);                              \
    _mrb_thash_probe((name_ *)t, hash_func_(key), idx, {                      \
      if (keys[idx] == empty_key_) return _##name_##_it_null();               \
      if (keys[idx] == deleted_key_) continue;                                \
      if (!equal_p_func_(key, keys[idx])) continue;                           \
      return _##name_##_it((name_ *)t, idx);                                  \
    });                                                                       \
    return _##name_##_it_null();                                              \
  }                                                                           \
                                                                              \
  /* Deletes the entry for the key from the map/set. */                       \
  void                                                                        \
  name_##_delete(mrb_state *mrb, name_ **tp, K key)                           \
  {                                                                           \
    if (!*tp) return;                                                         \
    name_##_iter it = name_##_get(*tp, key);                                  \
    if (!name_##_it_null_p(it)) name_##_delete_by_it(mrb, tp, it);            \
  }                                                                           \
                                                                              \
  /* Deletes the entry for the iterator from the map/set. */                  \
  void                                                                        \
  name_##_delete_by_it(mrb_state *mrb, name_ **tp, name_##_iter it)           \
  {                                                                           \
    _##name_##_it_delete(it);                                                 \
    _##name_##_set_size(*tp, _##name_##_size(*tp) - 1);                       \
  }                                                                           \
                                                                              \
  /* Iterates over the map/set. Breaks the loop If `func` returns non 0. */   \
  void                                                                        \
  name_##_each(mrb_state *mrb, name_ *t, name_##_each_func *f, void *d)       \
  {                                                                           \
    uint16_t size;                                                            \
    if (!t || (size = _##name_##_size(t)) == 0) return;                       \
    K *keys = _##name_##_keys(t);                                             \
    for (uint16_t idx = 0; size; ++idx) {                                     \
      if (!active_key_p_func_(keys[idx])) continue;                           \
      if (f(mrb, (name_##_iter){t, idx}, d) != 0) return;                     \
      --size;                                                                 \
    }                                                                         \
  }                                                                           \
                                                                              \
  /* Copy the map/set. */                                                     \
  name_ *                                                                     \
  name_##_copy(mrb_state *mrb, const name_ *t)                                \
  {                                                                           \
    if (!t) return NULL;                                                      \
    uint32_t capa = _##name_##_capa(t);                                       \
    size_t offset = _##name_##_offset_for(capa);                              \
    size_t memsize = _##name_##_memsize_for(capa);                            \
    void *p = mrb_malloc(mrb, memsize);                                       \
    memcpy(p, (char *)t - offset, memsize);                                   \
    return (name_ *)((char *)p + offset);                                     \
  }                                                                           \
                                                                              \
  /* Free the map/set. */                                                     \
  void                                                                        \
  name_##_free(mrb_state *mrb, name_ *t)                                      \
  {                                                                           \
    if (t) _##name_##_free(mrb, t);                                           \
  }

#define MRB_THASH_INIT_CAPA 1
#define MRB_THASH_MAX_CAPA ((uint32_t)UINT16_MAX + 1)
#define MRB_THASH_MAX_SIZE UINT16_MAX

#define _mrb_thash_size(t) ((t)->size)
#define _mrb_thash_set_size(t, size) ((t)->size = size)
#define _mrb_thash_capa(t) ((uint32_t)(t)->capa_1 + 1)
#define _mrb_thash_set_capa(t, capa) ((t)->capa_1 = (uint16_t)((capa) - 1))

#define _mrb_thash_probe(t, hash_code, idx_var, code) do {                    \
  uint32_t capa__ = _mrb_thash_capa(t);                                       \
  uint16_t idx_var = (hash_code) & (t)->capa_1;                               \
  for (; capa__--; idx_var = (idx_var + 1) & (t)->capa_1) {                   \
    code;                                                                     \
  }                                                                           \
} while (0)

typedef struct {} mrb_tset_pseudo_value;

#endif  /* MRUBY_THASH_H */
