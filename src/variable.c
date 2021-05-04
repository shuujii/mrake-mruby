/*
** variable.c - mruby variables
**
** See Copyright Notice in mruby.h
*/

#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/proc.h>
#include <mruby/smap.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <mruby/presym.h>

static int
iv_mark_i(mrb_state *mrb, mrb_sym sym, mrb_value v, void *p)
{
  mrb_gc_mark_value(mrb, v);
  return 0;
}

static void
mark_tbl(mrb_state *mrb, mrb_smap *t)
{
  mrb_smap_each_pair(mrb, t, iv_mark_i, 0);
}

void
mrb_gc_mark_gv(mrb_state *mrb)
{
  mark_tbl(mrb, mrb->globals);
}

void
mrb_gc_free_gv(mrb_state *mrb)
{
  mrb_smap_free(mrb, mrb->globals);
}

void
mrb_gc_mark_iv(mrb_state *mrb, struct RObject *obj)
{
  mark_tbl(mrb, obj->iv);
}

size_t
mrb_gc_mark_iv_size(mrb_state *mrb, struct RObject *obj)
{
  return mrb_smap_size(obj->iv);
}

void
mrb_gc_free_iv(mrb_state *mrb, struct RObject *obj)
{
  mrb_smap_free(mrb, obj->iv);
}

mrb_value
mrb_vm_special_get(mrb_state *mrb, mrb_sym i)
{
  return mrb_fixnum_value(0);
}

void
mrb_vm_special_set(mrb_state *mrb, mrb_sym i, mrb_value v)
{
}

static mrb_bool
obj_iv_p(mrb_value obj)
{
  switch (mrb_type(obj)) {
    case MRB_TT_OBJECT:
    case MRB_TT_CLASS:
    case MRB_TT_MODULE:
    case MRB_TT_SCLASS:
    case MRB_TT_HASH:
    case MRB_TT_DATA:
    case MRB_TT_EXCEPTION:
      return TRUE;
    default:
      return FALSE;
  }
}

static mrb_smap**
class_iv_ptr(struct RClass *c)
{
  return c->tt == MRB_TT_ICLASS ? &c->iv_c->iv : &c->iv;
}

MRB_API mrb_value
mrb_obj_iv_get(mrb_state *mrb, struct RObject *obj, mrb_sym sym)
{
  mrb_smap_iter it = mrb_smap_get(obj->iv, sym);
  return mrb_smap_it_null_p(it) ? mrb_nil_value() : mrb_smap_it_value(it);
}

MRB_API mrb_value
mrb_iv_get(mrb_state *mrb, mrb_value obj, mrb_sym sym)
{
  if (obj_iv_p(obj)) {
    return mrb_obj_iv_get(mrb, mrb_obj_ptr(obj), sym);
  }
  return mrb_nil_value();
}

static inline void assign_class_name(mrb_state *mrb, struct RObject *obj, mrb_sym sym, mrb_value v);

void
mrb_obj_iv_set_force(mrb_state *mrb, struct RObject *obj, mrb_sym sym, mrb_value v)
{
  assign_class_name(mrb, obj, sym, v);
  mrb_smap_set(mrb, &obj->iv, sym, v);
  mrb_field_write_barrier_value(mrb, (struct RBasic*)obj, v);
}

MRB_API void
mrb_obj_iv_set(mrb_state *mrb, struct RObject *obj, mrb_sym sym, mrb_value v)
{
  mrb_check_frozen(mrb, obj);
  mrb_obj_iv_set_force(mrb, obj, sym, v);
}

/* Iterates over the instance variable table. */
MRB_API void
mrb_iv_foreach(mrb_state *mrb, mrb_value obj, mrb_iv_foreach_func *func, void *p)
{
  if (!obj_iv_p(obj)) return;
  mrb_smap_each_pair(mrb, mrb_obj_ptr(obj)->iv, func, p);
}

static inline mrb_bool
namespace_p(enum mrb_vtype tt)
{
  return tt == MRB_TT_CLASS || tt == MRB_TT_MODULE ? TRUE : FALSE;
}

static inline void
assign_class_name(mrb_state *mrb, struct RObject *obj, mrb_sym sym, mrb_value v)
{
  if (namespace_p(obj->tt) && namespace_p(mrb_type(v))) {
    struct RObject *c = mrb_obj_ptr(v);
    if (obj != c && ISUPPER(mrb_sym_name_len(mrb, sym, NULL)[0])) {
      mrb_sym id_classname = MRB_SYM(__classname__);
      mrb_value o = mrb_obj_iv_get(mrb, c, id_classname);

      if (mrb_nil_p(o)) {
        mrb_sym id_outer = MRB_SYM(__outer__);
        o = mrb_obj_iv_get(mrb, c, id_outer);

        if (mrb_nil_p(o)) {
          if ((struct RClass *)obj == mrb->object_class) {
            mrb_obj_iv_set_force(mrb, c, id_classname, mrb_symbol_value(sym));
          }
          else {
            mrb_obj_iv_set_force(mrb, c, id_outer, mrb_obj_value(obj));
          }
        }
      }
    }
  }
}

MRB_API void
mrb_iv_set(mrb_state *mrb, mrb_value obj, mrb_sym sym, mrb_value v)
{
  if (obj_iv_p(obj)) {
    mrb_obj_iv_set(mrb, mrb_obj_ptr(obj), sym, v);
  }
  else {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "cannot set instance variable");
  }
}

MRB_API mrb_bool
mrb_obj_iv_defined(mrb_state *mrb, struct RObject *obj, mrb_sym sym)
{
  return mrb_smap_include_p(obj->iv, sym);
}

MRB_API mrb_bool
mrb_iv_defined(mrb_state *mrb, mrb_value obj, mrb_sym sym)
{
  if (!obj_iv_p(obj)) return FALSE;
  return mrb_obj_iv_defined(mrb, mrb_obj_ptr(obj), sym);
}

MRB_API mrb_bool
mrb_iv_name_sym_p(mrb_state *mrb, mrb_sym iv_name)
{
  const char *s;
  mrb_int len;

  s = mrb_sym_name_len(mrb, iv_name, &len);
  if (len < 2) return FALSE;
  if (s[0] != '@') return FALSE;
  if (ISDIGIT(s[1])) return FALSE;
  return mrb_ident_p(s+1, len-1);
}

MRB_API void
mrb_iv_name_sym_check(mrb_state *mrb, mrb_sym iv_name)
{
  if (!mrb_iv_name_sym_p(mrb, iv_name)) {
    mrb_name_error(mrb, iv_name, "'%n' is not allowed as an instance variable name", iv_name);
  }
}

MRB_API void
mrb_iv_copy(mrb_state *mrb, mrb_value dest, mrb_value src)
{
  struct RObject *d = mrb_obj_ptr(dest);
  struct RObject *s = mrb_obj_ptr(src);

  mrb_smap *t = mrb_smap_copy(mrb, s->iv);
  mrb_smap_free(mrb, d->iv);
  if (t) mrb_write_barrier(mrb, (struct RBasic*)d);
  d->iv = t;
}

static int
inspect_i(mrb_state *mrb, mrb_sym sym, mrb_value v, void *p)
{
  mrb_value str = *(mrb_value*)p;
  const char *s;
  mrb_int len;
  mrb_value ins;
  char *sp = RSTRING_PTR(str);

  /* need not to show internal data */
  if (sp[0] == '-') { /* first element */
    sp[0] = '#';
    mrb_str_cat_lit(mrb, str, " ");
  }
  else {
    mrb_str_cat_lit(mrb, str, ", ");
  }
  s = mrb_sym_name_len(mrb, sym, &len);
  mrb_str_cat(mrb, str, s, len);
  mrb_str_cat_lit(mrb, str, "=");
  if (mrb_object_p(v)) {
    ins = mrb_any_to_s(mrb, v);
  }
  else {
    ins = mrb_inspect(mrb, v);
  }
  mrb_str_cat_str(mrb, str, ins);
  return 0;
}

mrb_value
mrb_obj_iv_inspect(mrb_state *mrb, struct RObject *obj)
{
  if (mrb_smap_size(obj->iv) > 0) {
    const char *cn = mrb_obj_classname(mrb, mrb_obj_value(obj));
    mrb_value str = mrb_str_new_capa(mrb, 30);

    mrb_str_cat_lit(mrb, str, "-<");
    mrb_str_cat_cstr(mrb, str, cn);
    mrb_str_cat_lit(mrb, str, ":");
    mrb_str_cat_str(mrb, str, mrb_ptr_to_str(mrb, obj));

    mrb_smap_each_pair(mrb, obj->iv, inspect_i, &str);
    mrb_str_cat_lit(mrb, str, ">");
    return str;
  }
  return mrb_any_to_s(mrb, mrb_obj_value(obj));
}

MRB_API mrb_value
mrb_iv_remove(mrb_state *mrb, mrb_value obj, mrb_sym sym)
{
  if (obj_iv_p(obj)) {
    mrb_value val;
    mrb_check_frozen(mrb, mrb_obj_ptr(obj));
    if (mrb_smap_delete2(mrb, &mrb_obj_ptr(obj)->iv, sym, &val)) return val;
  }
  return mrb_undef_value();
}

static int
iv_i(mrb_state *mrb, mrb_sym sym, mrb_value v, void *p)
{
  mrb_value ary;
  const char* s;
  mrb_int len;

  ary = *(mrb_value*)p;
  s = mrb_sym_name_len(mrb, sym, &len);
  if (len > 1 && s[0] == '@' && s[1] != '@') {
    mrb_ary_push(mrb, ary, mrb_symbol_value(sym));
  }
  return 0;
}

/* 15.3.1.3.23 */
/*
 *  call-seq:
 *     obj.instance_variables    -> array
 *
 *  Returns an array of instance variable names for the receiver. Note
 *  that simply defining an accessor does not create the corresponding
 *  instance variable.
 *
 *     class Fred
 *       attr_accessor :a1
 *       def initialize
 *         @iv = 3
 *       end
 *     end
 *     Fred.new.instance_variables   #=> [:@iv]
 */
mrb_value
mrb_obj_instance_variables(mrb_state *mrb, mrb_value self)
{
  mrb_value ary;

  ary = mrb_ary_new(mrb);
  if (obj_iv_p(self)) {
    mrb_smap_each_pair(mrb, mrb_obj_ptr(self)->iv, iv_i, &ary);
  }
  return ary;
}

static int
cv_i(mrb_state *mrb, mrb_sym sym, mrb_value v, void *p)
{
  mrb_value ary;
  const char* s;
  mrb_int len;

  ary = *(mrb_value*)p;
  s = mrb_sym_name_len(mrb, sym, &len);
  if (len > 2 && s[0] == '@' && s[1] == '@') {
    mrb_ary_push(mrb, ary, mrb_symbol_value(sym));
  }
  return 0;
}

/* 15.2.2.4.19 */
/*
 *  call-seq:
 *     mod.class_variables(inherit=true)   -> array
 *
 *  Returns an array of the names of class variables in <i>mod</i>.
 *
 *     class One
 *       @@var1 = 1
 *     end
 *     class Two < One
 *       @@var2 = 2
 *     end
 *     One.class_variables   #=> [:@@var1]
 *     Two.class_variables   #=> [:@@var2]
 */
mrb_value
mrb_mod_class_variables(mrb_state *mrb, mrb_value mod)
{
  mrb_value ary;
  struct RClass *c;
  mrb_bool inherit = TRUE;

  mrb_get_args(mrb, "|b", &inherit);
  ary = mrb_ary_new(mrb);
  c = mrb_class_ptr(mod);
  mrb_smap_each_pair(mrb, c->iv, cv_i, &ary);
  if (inherit) {
    while ((c = c->super)) {
      mrb_smap_each_pair(mrb, *class_iv_ptr(c), cv_i, &ary);
    }
  }
  return ary;
}

mrb_value
mrb_mod_cv_get(mrb_state *mrb, struct RClass *c, mrb_sym sym)
{
  struct RClass * cls = c;
  mrb_value v;
  int given = FALSE;

  do {
    if (mrb_smap_get2(*class_iv_ptr(c), sym, &v)) given = TRUE;
  } while ((c = c->super));
  if (given) return v;
  if (cls->tt == MRB_TT_SCLASS) {
    mrb_value klass;
    klass = mrb_obj_iv_get(mrb, (struct RObject *)cls, MRB_SYM(__attached__));
    c = mrb_class_ptr(klass);
    if (c->tt == MRB_TT_CLASS || c->tt == MRB_TT_MODULE) {
      do {
        if (mrb_smap_get2(*class_iv_ptr(c), sym, &v)) given = TRUE;
      } while ((c = c->super));
      if (given) return v;
    }
  }
  mrb_name_error(mrb, sym, "uninitialized class variable %n in %C", sym, cls);
  /* not reached */
  return mrb_nil_value();
}

MRB_API mrb_value
mrb_cv_get(mrb_state *mrb, mrb_value mod, mrb_sym sym)
{
  return mrb_mod_cv_get(mrb, mrb_class_ptr(mod), sym);
}

MRB_API void
mrb_mod_cv_set(mrb_state *mrb, struct RClass *c, mrb_sym sym, mrb_value v)
{
  struct RClass * cls = c;

  do {
    mrb_smap **iv_ptr = class_iv_ptr(c);
    mrb_smap_iter it = mrb_smap_get(*iv_ptr, sym);
    if (!mrb_smap_it_null_p(it)) {
      mrb_check_frozen(mrb, c);
      mrb_smap_it_set_value(it, v);
      mrb_field_write_barrier_value(mrb, (struct RBasic*)c, v);
      return;
    }
  } while ((c = c->super));

  if (cls->tt == MRB_TT_SCLASS) {
    mrb_value klass;

    klass = mrb_obj_iv_get(mrb, (struct RObject*)cls, MRB_SYM(__attached__));
    switch (mrb_type(klass)) {
    case MRB_TT_CLASS:
    case MRB_TT_MODULE:
    case MRB_TT_SCLASS:
      c = mrb_class_ptr(klass);
      break;
    default:
      c = cls;
      break;
    }
  }
  else{
    c = cls;
  }

  mrb_check_frozen(mrb, c);
  mrb_smap_set(mrb, class_iv_ptr(c), sym, v);
  mrb_field_write_barrier_value(mrb, (struct RBasic*)c, v);
}

MRB_API void
mrb_cv_set(mrb_state *mrb, mrb_value mod, mrb_sym sym, mrb_value v)
{
  mrb_mod_cv_set(mrb, mrb_class_ptr(mod), sym, v);
}

mrb_bool
mrb_mod_cv_defined(mrb_state *mrb, struct RClass * c, mrb_sym sym)
{
  do {
    if (mrb_smap_include_p(*class_iv_ptr(c), sym)) return TRUE;
  } while ((c = c->super));

  return FALSE;
}

MRB_API mrb_bool
mrb_cv_defined(mrb_state *mrb, mrb_value mod, mrb_sym sym)
{
  return mrb_mod_cv_defined(mrb, mrb_class_ptr(mod), sym);
}

mrb_value
mrb_vm_cv_get(mrb_state *mrb, mrb_sym sym)
{
  struct RClass *c;

  const struct RProc *p = mrb->c->ci->proc;

  for (;;) {
    c = MRB_PROC_TARGET_CLASS(p);
    if (c && c->tt != MRB_TT_SCLASS) break;
    p = p->upper;
  }
  return mrb_mod_cv_get(mrb, c, sym);
}

void
mrb_vm_cv_set(mrb_state *mrb, mrb_sym sym, mrb_value v)
{
  struct RClass *c;
  const struct RProc *p = mrb->c->ci->proc;

  for (;;) {
    c = MRB_PROC_TARGET_CLASS(p);
    if (c && c->tt != MRB_TT_SCLASS) break;
    p = p->upper;
  }
  mrb_mod_cv_set(mrb, c, sym, v);
}

static void
mod_const_check(mrb_state *mrb, mrb_value mod)
{
  switch (mrb_type(mod)) {
  case MRB_TT_CLASS:
  case MRB_TT_MODULE:
  case MRB_TT_SCLASS:
    break;
  default:
    mrb_raise(mrb, E_TYPE_ERROR, "constant look-up for non class/module");
    break;
  }
}

static mrb_value
const_get(mrb_state *mrb, struct RClass *base, mrb_sym sym)
{
  struct RClass *c = base;
  mrb_bool retry = FALSE;
  mrb_value v, name;

L_RETRY:
  do {
    if (mrb_smap_get2(*class_iv_ptr(c), sym, &v)) return v;
  } while ((c = c->super));
  if (!retry && base->tt == MRB_TT_MODULE) {
    c = mrb->object_class;
    retry = TRUE;
    goto L_RETRY;
  }
  name = mrb_symbol_value(sym);
  return mrb_funcall_argv(mrb, mrb_obj_value(base), MRB_SYM(const_missing), 1, &name);
}

MRB_API mrb_value
mrb_const_get(mrb_state *mrb, mrb_value mod, mrb_sym sym)
{
  mod_const_check(mrb, mod);
  return const_get(mrb, mrb_class_ptr(mod), sym);
}

mrb_value
mrb_vm_const_get(mrb_state *mrb, mrb_sym sym)
{
  struct RClass *c;
  struct RClass *c2;
  mrb_value v;
  const struct RProc *proc;

  c = MRB_PROC_TARGET_CLASS(mrb->c->ci->proc);
  if (!c) c = mrb->object_class;
  if (mrb_smap_get2(c->iv, sym, &v)) return v;
  c2 = c;
  while (c2 && c2->tt == MRB_TT_SCLASS) {
    mrb_value klass;
    if (!mrb_smap_get2(c2->iv, MRB_SYM(__attached__), &klass)) {
      c2 = NULL;
      break;
    }
    c2 = mrb_class_ptr(klass);
  }
  if (c2 && (c2->tt == MRB_TT_CLASS || c2->tt == MRB_TT_MODULE)) c = c2;
  mrb_assert(!MRB_PROC_CFUNC_P(mrb->c->ci->proc));
  proc = mrb->c->ci->proc;
  while (proc) {
    c2 = MRB_PROC_TARGET_CLASS(proc);
    if (c2 && mrb_smap_get2(c2->iv, sym, &v)) return v;
    proc = proc->upper;
  }
  return const_get(mrb, c, sym);
}

MRB_API void
mrb_const_set(mrb_state *mrb, mrb_value mod, mrb_sym sym, mrb_value v)
{
  mod_const_check(mrb, mod);
  if (mrb_type(v) == MRB_TT_CLASS || mrb_type(v) == MRB_TT_MODULE) {
    mrb_class_name_class(mrb, mrb_class_ptr(mod), mrb_class_ptr(v), sym);
  }
  mrb_iv_set(mrb, mod, sym, v);
}

void
mrb_vm_const_set(mrb_state *mrb, mrb_sym sym, mrb_value v)
{
  struct RClass *c;

  c = MRB_PROC_TARGET_CLASS(mrb->c->ci->proc);
  if (!c) c = mrb->object_class;
  mrb_obj_iv_set(mrb, (struct RObject*)c, sym, v);
}

MRB_API void
mrb_const_remove(mrb_state *mrb, mrb_value mod, mrb_sym sym)
{
  mod_const_check(mrb, mod);
  mrb_iv_remove(mrb, mod, sym);
}

MRB_API void
mrb_define_const_id(mrb_state *mrb, struct RClass *mod, mrb_sym name, mrb_value v)
{
  mrb_obj_iv_set(mrb, (struct RObject*)mod, name, v);
}

MRB_API void
mrb_define_const(mrb_state *mrb, struct RClass *mod, const char *name, mrb_value v)
{
  mrb_obj_iv_set(mrb, (struct RObject*)mod, mrb_intern_cstr(mrb, name), v);
}

MRB_API void
mrb_define_global_const(mrb_state *mrb, const char *name, mrb_value val)
{
  mrb_define_const(mrb, mrb->object_class, name, val);
}

static int
const_i(mrb_state *mrb, mrb_sym sym, mrb_value v, void *p)
{
  mrb_value ary;
  const char* s;
  mrb_int len;

  ary = *(mrb_value*)p;
  s = mrb_sym_name_len(mrb, sym, &len);
  if (len >= 1 && ISUPPER(s[0])) {
    mrb_int i, alen = RARRAY_LEN(ary);

    for (i=0; i<alen; i++) {
      if (mrb_symbol(RARRAY_PTR(ary)[i]) == sym)
        break;
    }
    if (i==alen) {
      mrb_ary_push(mrb, ary, mrb_symbol_value(sym));
    }
  }
  return 0;
}

/* 15.2.2.4.24 */
/*
 *  call-seq:
 *     mod.constants    -> array
 *
 *  Returns an array of all names of constants defined in the receiver.
 */
mrb_value
mrb_mod_constants(mrb_state *mrb, mrb_value mod)
{
  mrb_value ary;
  mrb_bool inherit = TRUE;
  struct RClass *c = mrb_class_ptr(mod);

  mrb_get_args(mrb, "|b", &inherit);
  ary = mrb_ary_new(mrb);
  mrb_smap_each_pair(mrb, c->iv, const_i, &ary);
  if (inherit) {
    while ((c = c->super) && c != mrb->object_class) {
      mrb_smap_each_pair(mrb, *class_iv_ptr(c), const_i, &ary);
    }
  }
  return ary;
}

MRB_API mrb_value
mrb_gv_get(mrb_state *mrb, mrb_sym sym)
{
  mrb_value v;
  return mrb_smap_get2(mrb->globals, sym, &v) ? v : mrb_nil_value();
}

MRB_API void
mrb_gv_set(mrb_state *mrb, mrb_sym sym, mrb_value v)
{
  mrb_smap_set(mrb, &mrb->globals, sym, v);
}

MRB_API void
mrb_gv_remove(mrb_state *mrb, mrb_sym sym)
{
  mrb_smap_delete(mrb, &mrb->globals, sym);
}

static int
gv_i(mrb_state *mrb, mrb_sym sym, mrb_value v, void *p)
{
  mrb_value ary;

  ary = *(mrb_value*)p;
  mrb_ary_push(mrb, ary, mrb_symbol_value(sym));
  return 0;
}

/* 15.3.1.2.4  */
/* 15.3.1.3.14 */
/*
 *  call-seq:
 *     global_variables    -> array
 *
 *  Returns an array of the names of global variables.
 *
 *     global_variables.grep /std/   #=> [:$stdin, :$stdout, :$stderr]
 */
mrb_value
mrb_f_global_variables(mrb_state *mrb, mrb_value self)
{
  mrb_value ary = mrb_ary_new(mrb);
  mrb_smap_each_pair(mrb, mrb->globals, gv_i, &ary);
  return ary;
}

static mrb_bool
mrb_const_defined_0(mrb_state *mrb, mrb_value mod, mrb_sym id, mrb_bool exclude, mrb_bool recurse)
{
  struct RClass *klass = mrb_class_ptr(mod);
  struct RClass *tmp;
  mrb_bool mod_retry = FALSE;

  tmp = klass;
retry:
  if (mrb_smap_include_p(tmp->iv, id)) return TRUE;
  if (recurse || klass == mrb->object_class) {
    while ((tmp = tmp->super)) {
      if (mrb_smap_include_p(*class_iv_ptr(tmp), id)) return TRUE;
    }
  }
  if (!exclude && !mod_retry && (klass->tt == MRB_TT_MODULE)) {
    mod_retry = TRUE;
    tmp = mrb->object_class;
    goto retry;
  }
  return FALSE;
}

MRB_API mrb_bool
mrb_const_defined(mrb_state *mrb, mrb_value mod, mrb_sym id)
{
  return mrb_const_defined_0(mrb, mod, id, TRUE, TRUE);
}

MRB_API mrb_bool
mrb_const_defined_at(mrb_state *mrb, mrb_value mod, mrb_sym id)
{
  return mrb_const_defined_0(mrb, mod, id, TRUE, FALSE);
}

MRB_API mrb_value
mrb_attr_get(mrb_state *mrb, mrb_value obj, mrb_sym id)
{
  return mrb_iv_get(mrb, obj, id);
}

struct csym_arg {
  struct RClass *c;
  mrb_sym sym;
};

static int
csym_i(mrb_state *mrb, mrb_sym sym, mrb_value v, void *p)
{
  struct csym_arg *a = (struct csym_arg*)p;
  struct RClass *c = a->c;

  if (mrb_type(v) == c->tt && mrb_class_ptr(v) == c) {
    a->sym = sym;
    return 1;     /* stop iteration */
  }
  return 0;
}

static mrb_sym
find_class_sym(mrb_state *mrb, struct RClass *outer, struct RClass *c)
{
  struct csym_arg arg;

  if (!outer) return 0;
  if (outer == c) return 0;
  arg.c = c;
  arg.sym = 0;
  mrb_smap_each_pair(mrb, outer->iv, csym_i, &arg);
  return arg.sym;
}

static struct RClass*
outer_class(mrb_state *mrb, struct RClass *c)
{
  mrb_value ov;

  ov = mrb_obj_iv_get(mrb, (struct RObject*)c, MRB_SYM(__outer__));
  if (mrb_nil_p(ov)) return NULL;
  switch (mrb_type(ov)) {
  case MRB_TT_CLASS:
  case MRB_TT_MODULE:
    return mrb_class_ptr(ov);
  default:
    break;
  }
  return NULL;
}

static mrb_bool
detect_outer_loop(mrb_state *mrb, struct RClass *c)
{
  struct RClass *t = c;         /* tortoise */
  struct RClass *h = c;         /* hare */

  for (;;) {
    if (h == NULL) return FALSE;
    h = outer_class(mrb, h);
    if (h == NULL) return FALSE;
    h = outer_class(mrb, h);
    t = outer_class(mrb, t);
    if (t == h) return TRUE;
  }
}

mrb_value
mrb_class_find_path(mrb_state *mrb, struct RClass *c)
{
  struct RClass *outer;
  mrb_value path;
  mrb_sym name;
  const char *str;
  mrb_int len;

  if (detect_outer_loop(mrb, c)) return mrb_nil_value();
  outer = outer_class(mrb, c);
  if (outer == NULL) return mrb_nil_value();
  name = find_class_sym(mrb, outer, c);
  if (name == 0) return mrb_nil_value();
  str = mrb_class_name(mrb, outer);
  path = mrb_str_new_capa(mrb, 40);
  mrb_str_cat_cstr(mrb, path, str);
  mrb_str_cat_cstr(mrb, path, "::");

  str = mrb_sym_name_len(mrb, name, &len);
  mrb_str_cat(mrb, path, str, len);
  if (RSTRING_PTR(path)[0] != '#') {
    mrb_smap_delete(mrb, &c->iv, MRB_SYM(__outer__));
    mrb_smap_set(mrb, &c->iv, MRB_SYM(__classname__), path);
    mrb_field_write_barrier_value(mrb, (struct RBasic*)c, path);
    path = mrb_str_dup(mrb, path);
  }
  return path;
}

size_t
mrb_obj_iv_tbl_memsize(mrb_value obj)
{
  return mrb_smap_memsize(mrb_obj_ptr(obj)->iv);
}

#define identchar(c) (ISALNUM(c) || (c) == '_' || !ISASCII(c))

mrb_bool
mrb_ident_p(const char *s, mrb_int len)
{
  mrb_int i;

  for (i = 0; i < len; i++) {
    if (!identchar(s[i])) return FALSE;
  }
  return TRUE;
}
