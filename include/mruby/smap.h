#ifndef MRUBY_SMAP_H
#define MRUBY_SMAP_H

/*
 * thash based `mrb_sym` to `mrb_value` map
 */

#include <mruby/thash.h>

MRB_TMAP_DECLARE(mrb_smap, mrb_sym, mrb_value)

typedef int (mrb_smap_each_pair_func)(mrb_state *, mrb_sym, mrb_value, void *);

mrb_bool mrb_smap_get2(const mrb_smap *t, mrb_sym sym, mrb_value *valp);
mrb_bool mrb_smap_delete2(mrb_state *mrb, mrb_smap **tp, mrb_sym sym, mrb_value *valp);
void mrb_smap_each_pair(mrb_state *mrb, mrb_smap *t, mrb_smap_each_pair_func *func, void *data);

#endif  /* MRUBY_SMAP_H */
