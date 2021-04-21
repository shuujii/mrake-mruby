/*
** etc.c
**
** See Copyright Notice in mruby.h
*/

#include <mruby.h>
#include <mruby/string.h>
#include <mruby/data.h>
#include <mruby/class.h>
#include <mruby/numeric.h>

MRB_API struct RData*
mrb_data_object_alloc(mrb_state *mrb, struct RClass *klass, void *ptr, const mrb_data_type *type)
{
  struct RData *data;

  data = (struct RData*)mrb_obj_alloc(mrb, MRB_TT_DATA, klass);
  data->data = ptr;
  data->type = type;

  return data;
}

MRB_API void
mrb_data_check_type(mrb_state *mrb, mrb_value obj, const mrb_data_type *type)
{
  if (!mrb_data_p(obj)) {
    mrb_check_type(mrb, obj, MRB_TT_DATA);
  }
  if (DATA_TYPE(obj) != type) {
    const mrb_data_type *t2 = DATA_TYPE(obj);

    if (t2) {
      mrb_raisef(mrb, E_TYPE_ERROR, "wrong argument type %s (expected %s)",
                 t2->struct_name, type->struct_name);
    }
    else {
      mrb_raisef(mrb, E_TYPE_ERROR, "uninitialized %t (expected %s)",
                 obj, type->struct_name);
    }
  }
}

MRB_API void*
mrb_data_check_get_ptr(mrb_state *mrb, mrb_value obj, const mrb_data_type *type)
{
  if (!mrb_data_p(obj)) {
    return NULL;
  }
  if (DATA_TYPE(obj) != type) {
    return NULL;
  }
  return DATA_PTR(obj);
}

MRB_API void*
mrb_data_get_ptr(mrb_state *mrb, mrb_value obj, const mrb_data_type *type)
{
  mrb_data_check_type(mrb, obj, type);
  return DATA_PTR(obj);
}

MRB_API mrb_sym
mrb_obj_to_sym(mrb_state *mrb, mrb_value name)
{
  if (mrb_symbol_p(name)) return mrb_symbol(name);
  if (mrb_string_p(name)) return mrb_intern_str(mrb, name);
  mrb_raisef(mrb, E_TYPE_ERROR, "%!v is not a symbol nor a string", name);
  return 0;  /* not reached */
}

static mrb_int
make_num_id(const char *p, size_t len)
{
  uint32_t id = 0;

  while (len--) {
    id = id*65599 + *p;
    p++;
  }
  id = id + (id>>5);

  return (mrb_int)id;
}

MRB_API mrb_int
mrb_int_id(mrb_int n)
{
  return make_num_id((const char*)&n, sizeof(n));
}

#ifndef MRB_NO_FLOAT
MRB_API mrb_int
mrb_float_id(mrb_float f)
{
  /* normalize -0.0 to 0.0 */
  if (f == 0) f = 0.0;
  return make_num_id((const char*)&f, sizeof(f));
}
#endif

MRB_API mrb_int
mrb_obj_id(mrb_value obj)
{
  mrb_int tt = mrb_type(obj);

#define MakeID2(p,t) (mrb_int)(((intptr_t)(p))^(t))
#define MakeID(p)    MakeID2(p,tt)

  switch (tt) {
  case MRB_TT_FREE:
  case MRB_TT_UNDEF:
    return MakeID(0); /* not define */
  case MRB_TT_FALSE:
    if (mrb_nil_p(obj))
      return MakeID(4);
    else
      return MakeID(0);
  case MRB_TT_TRUE:
    return MakeID(2);
  case MRB_TT_SYMBOL:
    return MakeID(mrb_symbol(obj));
  case MRB_TT_INTEGER:
    return MakeID(mrb_int_id(mrb_integer(obj)));
#ifndef MRB_NO_FLOAT
  case MRB_TT_FLOAT:
    return MakeID(mrb_float_id(mrb_float(obj)));
#endif
  case MRB_TT_STRING:
  case MRB_TT_OBJECT:
  case MRB_TT_CLASS:
  case MRB_TT_MODULE:
  case MRB_TT_ICLASS:
  case MRB_TT_SCLASS:
  case MRB_TT_PROC:
  case MRB_TT_ARRAY:
  case MRB_TT_HASH:
  case MRB_TT_RANGE:
  case MRB_TT_EXCEPTION:
  case MRB_TT_DATA:
  case MRB_TT_ISTRUCT:
  default:
    return MakeID(mrb_ptr(obj));
  }
}

#if defined(MRB_NAN_BOXING) && defined(MRB_64BIT)
#define mrb_xxx_boxing_cptr_value mrb_nan_boxing_cptr_value
#endif

#ifdef MRB_WORD_BOXING
#define mrb_xxx_boxing_cptr_value mrb_word_boxing_cptr_value

#ifndef MRB_NO_FLOAT
MRB_API mrb_value
mrb_word_boxing_float_value(mrb_state *mrb, mrb_float f)
{
  struct RFloat *o = (struct RFloat*)mrb_obj_alloc(mrb, MRB_TT_FLOAT, mrb->float_class);
  o->f = f;
  MRB_SET_FROZEN_FLAG(o);
  return (mrb_value)o;
}
#endif  /* MRB_NO_FLOAT */

MRB_API mrb_value
mrb_word_boxing_int_value(mrb_state *mrb, mrb_int n)
{
  if (FIXABLE(n)) {
    return mrb_fixnum_value(n);
  }
  else {
    struct RInteger *o = (struct RInteger*)mrb_obj_alloc(mrb, MRB_TT_INTEGER, mrb->integer_class);
    o->i = n;
    MRB_SET_FROZEN_FLAG(o);
    return (mrb_value)o;
  }
}
#endif  /* MRB_WORD_BOXING */

#if defined(MRB_WORD_BOXING) || (defined(MRB_NAN_BOXING) && defined(MRB_64BIT))
MRB_API mrb_value
mrb_xxx_boxing_cptr_value(mrb_state *mrb, void *p)
{
  struct RCptr *o = (struct RCptr*)mrb_obj_alloc(mrb, MRB_TT_CPTR, mrb->object_class);
  o->p = p;
  return (mrb_value)o;
}
#endif

#if defined _MSC_VER && _MSC_VER < 1900

#ifndef va_copy
static void
mrb_msvc_va_copy(va_list *dest, va_list src)
{
  *dest = src;
}
#define va_copy(dest, src) mrb_msvc_va_copy(&(dest), src)
#endif

MRB_API int
mrb_msvc_vsnprintf(char *s, size_t n, const char *format, va_list arg)
{
  int cnt;
  va_list argcp;
  va_copy(argcp, arg);
  if (n == 0 || (cnt = _vsnprintf_s(s, n, _TRUNCATE, format, argcp)) < 0) {
    cnt = _vscprintf(format, arg);
  }
  va_end(argcp);
  return cnt;
}

MRB_API int
mrb_msvc_snprintf(char *s, size_t n, const char *format, ...)
{
  va_list arg;
  int ret;
  va_start(arg, format);
  ret = mrb_msvc_vsnprintf(s, n, format, arg);
  va_end(arg);
  return ret;
}

#endif  /* defined _MSC_VER && _MSC_VER < 1900 */
