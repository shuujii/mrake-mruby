/**
** @file mruby/boxing_word.h - word boxing mrb_value definition
**
** See Copyright Notice in mruby.h
*/

#ifndef MRUBY_BOXING_WORD_H
#define MRUBY_BOXING_WORD_H

#ifndef MRB_NO_FLOAT
struct RFloat {
  MRB_OBJECT_HEADER;
  mrb_float f;
};
#endif

struct RInteger {
  MRB_OBJECT_HEADER;
  mrb_int i;
};

/*
 * mrb_value representation:
 *
 *   nil   : 00000000 00000000 00000000 00000000 00000000 .... 00000000 [^1]
 *   false : 00000000 00000000 00000000 00000000 00000000 .... 00000100 [^2]
 *   true  : 00000000 00000000 00000000 00000000 00000000 .... 00001100
 *   undef : 00000000 00000000 00000000 00000000 00000000 .... 00010100
 *   fixnum: IIIIIIII IIIIIIII IIIIIIII IIIIIIII IIIIIIII .... IIIIIII1
 *  (flonum: EEEEEEEE EMMMMMMM MMMMMMMM MMMMMMMM MMMMMMMM .... MMMMMS10 [^3])
 *   symbol: YYYYYYYY YYYYYYYY YYYYYYYY YYYYYYYY 00000000 .... 00011100
 *   object: 00PPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP .... PPPPP000 [^4]
 *
 *   [^1] All bits are 0 (mrb_fixnum(v) == 0)
 *   [^2] mrb_fixnum(v) != 0
 *   [^3] Reserved
 *   [^4] Raw object pointer, any bit of P is 1
 */

typedef uintptr_t mrb_value;

#define BOXWORD_FIXNUM_FLAG     1
#define BOXWORD_FLONUM_FLAG     2
#define BOXWORD_SYMBOL_FLAG     28
#define BOXWORD_FIXNUM_SHIFT    1
#define BOXWORD_FLONUM_SHIFT    2
#define BOXWORD_SYMBOL_SHIFT    32
#define BOXWORD_FIXNUM_MASK     (((uintptr_t)1 << BOXWORD_FIXNUM_SHIFT) - 1)
#define BOXWORD_FLONUM_MASK     (((uintptr_t)1 << BOXWORD_FLONUM_SHIFT) - 1)
#define BOXWORD_SYMBOL_MASK     (((uintptr_t)1 << BOXWORD_SYMBOL_SHIFT) - 1)
#define BOXWORD_IMMEDIATE_MASK  (((uintptr_t)1 << 3) - 1)

#define MRB_FIXNUM_MIN (INT64_MIN >> BOXWORD_FIXNUM_SHIFT)
#define MRB_FIXNUM_MAX (INT64_MAX >> BOXWORD_FIXNUM_SHIFT)

#define BOXWORD_SET_SHIFT_VALUE(o,n,v) \
  ((o) = (((uintptr_t)(v)) << BOXWORD_##n##_SHIFT) | BOXWORD_##n##_FLAG)
#define BOXWORD_SHIFT_VALUE_P(o,n) \
  (((o) & BOXWORD_##n##_MASK) == BOXWORD_##n##_FLAG)
#define BOXWORD_OBJ_TYPE_P(o,n) \
  (!mrb_immediate_p(o) && ((struct RBasic*)(o))->tt == MRB_TT_##n)

MRB_API mrb_value mrb_word_boxing_cptr_value(struct mrb_state*, void*);
#ifndef MRB_NO_FLOAT
MRB_API mrb_value mrb_word_boxing_float_value(struct mrb_state*, mrb_float);
#endif
MRB_API mrb_value mrb_word_boxing_int_value(struct mrb_state*, mrb_int);

#define mrb_ptr(o) (void*)(o)
#define mrb_cptr(o) ((struct RCptr*)(o))->p
#ifndef MRB_NO_FLOAT
#define mrb_float(o) ((struct RFloat*)(o))->f
#endif
#define mrb_fixnum(o) (mrb_int)((intptr_t)(o) >> BOXWORD_FIXNUM_SHIFT)
#define mrb_integer(o) mrb_integer_func(o)
#define mrb_symbol(o) (mrb_sym)((o) >> BOXWORD_SYMBOL_SHIFT)
#define mrb_bool(o) (((o) & ~mrb_false_value()) != 0)

#define mrb_immediate_p(o) ((o) & BOXWORD_IMMEDIATE_MASK || mrb_nil_p(o))
#define mrb_undef_p(o) ((o) == mrb_undef_value())
#define mrb_nil_p(o)  ((o) == mrb_nil_value())
#define mrb_false_p(o) ((o) == mrb_false_value())
#define mrb_true_p(o)  ((o) == mrb_true_value())
#ifndef MRB_NO_FLOAT
#define mrb_float_p(o) BOXWORD_OBJ_TYPE_P(o, FLOAT)
#endif
#define mrb_fixnum_p(o) BOXWORD_SHIFT_VALUE_P(o, FIXNUM)
#define mrb_integer_p(o) (mrb_fixnum_p(o) || BOXWORD_OBJ_TYPE_P(o, INTEGER))
#define mrb_symbol_p(o) BOXWORD_SHIFT_VALUE_P(o, SYMBOL)
#define mrb_array_p(o) BOXWORD_OBJ_TYPE_P(o, ARRAY)
#define mrb_string_p(o) BOXWORD_OBJ_TYPE_P(o, STRING)
#define mrb_hash_p(o) BOXWORD_OBJ_TYPE_P(o, HASH)
#define mrb_cptr_p(o) BOXWORD_OBJ_TYPE_P(o, CPTR)
#define mrb_exception_p(o) BOXWORD_OBJ_TYPE_P(o, EXCEPTION)
#define mrb_free_p(o) BOXWORD_OBJ_TYPE_P(o, FREE)
#define mrb_object_p(o) BOXWORD_OBJ_TYPE_P(o, OBJECT)
#define mrb_class_p(o) BOXWORD_OBJ_TYPE_P(o, CLASS)
#define mrb_module_p(o) BOXWORD_OBJ_TYPE_P(o, MODULE)
#define mrb_iclass_p(o) BOXWORD_OBJ_TYPE_P(o, ICLASS)
#define mrb_sclass_p(o) BOXWORD_OBJ_TYPE_P(o, SCLASS)
#define mrb_proc_p(o) BOXWORD_OBJ_TYPE_P(o, PROC)
#define mrb_range_p(o) BOXWORD_OBJ_TYPE_P(o, RANGE)
#define mrb_env_p(o) BOXWORD_OBJ_TYPE_P(o, ENV)
#define mrb_data_p(o) BOXWORD_OBJ_TYPE_P(o, DATA)
#define mrb_fiber_p(o) BOXWORD_OBJ_TYPE_P(o, FIBER)
#define mrb_istruct_p(o) BOXWORD_OBJ_TYPE_P(o, ISTRUCT)
#define mrb_break_p(o) BOXWORD_OBJ_TYPE_P(o, BREAK)

#ifndef MRB_NO_FLOAT
#define SET_FLOAT_VALUE(mrb,r,v) ((r) = mrb_word_boxing_float_value(mrb, v))
#endif
#define SET_CPTR_VALUE(mrb,r,v) ((r) = mrb_word_boxing_cptr_value(mrb, v))
#define SET_UNDEF_VALUE(r) ((r) = mrb_undef_value())
#define SET_NIL_VALUE(r) ((r) = mrb_nil_value())
#define SET_FALSE_VALUE(r) ((r) = mrb_false_value())
#define SET_TRUE_VALUE(r) ((r) = mrb_true_value())
#define SET_BOOL_VALUE(r,b) ((b) ? SET_TRUE_VALUE(r) : SET_FALSE_VALUE(r))
#define SET_INT_VALUE(mrb,r,n) ((r) = mrb_word_boxing_int_value(mrb, n))
#define SET_FIXNUM_VALUE(r,n) BOXWORD_SET_SHIFT_VALUE(r, FIXNUM, n)
#define SET_SYM_VALUE(r,n) BOXWORD_SET_SHIFT_VALUE(r, SYMBOL, n)
#define SET_OBJ_VALUE(r,v) ((r) = (mrb_value)(v))

MRB_INLINE mrb_value mrb_obj_value(void *p) { return (mrb_value)p; }
MRB_INLINE mrb_value mrb_nil_value(void) { return 0; }
MRB_INLINE mrb_value mrb_false_value(void) { return 4; }
MRB_INLINE mrb_value mrb_true_value(void) { return 12; }
MRB_INLINE mrb_value mrb_undef_value(void) { return 20; }
MRB_INLINE mrb_value mrb_bool_value(mrb_bool b) { return b ? mrb_true_value() : mrb_false_value(); }

MRB_INLINE mrb_int
mrb_integer(mrb_value o)
{
  mrb_assert(mrb_immediate_p(o) ? mrb_fixnum_p(o) : TRUE);
  return mrb_fixnum_p(o) ? mrb_fixnum(o) : ((struct RInteger*)(o))->i;
}

MRB_INLINE enum mrb_vtype
mrb_type(mrb_value o)
{
  return !mrb_bool(o)    ? MRB_TT_FALSE :
         mrb_true_p(o)   ? MRB_TT_TRUE :
         mrb_fixnum_p(o) ? MRB_TT_INTEGER :
         mrb_symbol_p(o) ? MRB_TT_SYMBOL :
         mrb_undef_p(o)  ? MRB_TT_UNDEF :
         ((struct RBasic*)(o))->tt;
}

#endif  /* MRUBY_BOXING_WORD_H */
