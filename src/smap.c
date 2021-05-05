#include <mruby.h>
#include <mruby/smap.h>

#define SMAP_EMPTY_SYM UINT32_MAX
#define SMAP_DELETED_SYM (SMAP_EMPTY_SYM - 1)

#define sym_active_p(sym) ((sym) < SMAP_DELETED_SYM)
#define sym_hash_code(sym) (uint32_t)((sym) ^ ((sym) << 2) ^ ((sym) >> 2))
#define sym_equal_p(sym1, sym2) ((sym1) == (sym2))

MRB_TMAP_DEFINE(mrb_smap, mrb_sym, mrb_value, SMAP_EMPTY_SYM, SMAP_DELETED_SYM,
                sym_active_p, sym_hash_code, sym_equal_p)

typedef struct {
  mrb_smap_each_pair_func *func;
  void *data;
} each_pair_arg;

static int
each_pair_i(mrb_state *mrb, mrb_smap_iter it, void *data)
{
  each_pair_arg *arg = (each_pair_arg *)data;
  return arg->func(mrb, mrb_smap_it_key(it), mrb_smap_it_value(it), arg->data);
}

mrb_bool
mrb_smap_get2(const mrb_smap *t, mrb_sym sym, mrb_value *valp)
{
  mrb_smap_iter it = mrb_smap_get(t, sym);
  if (mrb_smap_it_null_p(it)) return FALSE;
  *valp = mrb_smap_it_value(it);
  return TRUE;
}

mrb_bool
mrb_smap_delete2(mrb_state *mrb, mrb_smap **tp, mrb_sym sym, mrb_value *valp)
{
  mrb_smap_iter it = mrb_smap_get(*tp, sym);
  if (mrb_smap_it_null_p(it)) return FALSE;
  *valp = mrb_smap_it_value(it);
  mrb_smap_delete_by_it(mrb, tp, it);
  return TRUE;
}

void
mrb_smap_each_pair(mrb_state *mrb, mrb_smap *t, mrb_smap_each_pair_func *func, void *data)
{
  mrb_smap_each(mrb, t, each_pair_i, &(each_pair_arg){func, data});
}
