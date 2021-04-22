#ifndef MRUBY_SMAP_H
#define MRUBY_SMAP_H

/*
 * Map implementation `mrb_sym` to `mrb_value`
 */

typedef struct mrb_smap mrb_smap;
/* return non zero to break the loop */
typedef int (mrb_smap_each_func)(mrb_state *, mrb_sym, mrb_value, void *);

size_t mrb_smap_memsize(const mrb_smap *t);
uint16_t mrb_smap_size(const mrb_smap *t);
void mrb_smap_set(mrb_state *mrb, mrb_smap **tp, mrb_sym sym, mrb_value val);
mrb_bool mrb_smap_get(const mrb_smap *t, mrb_sym sym, mrb_value *valp);
mrb_bool mrb_smap_delete(mrb_state *mrb, mrb_smap **tp, mrb_sym sym, mrb_value *valp);
void mrb_smap_each(mrb_state *mrb, mrb_smap *t, mrb_smap_each_func *func, void *data);
mrb_smap *mrb_smap_copy(mrb_state *mrb, const mrb_smap *t);
void mrb_smap_free(mrb_state *mrb, mrb_smap *t);

#endif  /* MRUBY_SMAP_H */
