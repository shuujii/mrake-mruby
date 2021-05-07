/*
** backtrace.c -
**
** See Copyright Notice in mruby.h
*/

#include <mruby.h>
#include <mruby/variable.h>
#include <mruby/proc.h>
#include <mruby/array.h>
#include <mruby/string.h>
#include <mruby/class.h>
#include <mruby/debug.h>
#include <mruby/error.h>
#include <mruby/numeric.h>
#include <mruby/data.h>
#include <mruby/presym.h>

struct backtrace_location {
  int32_t lineno;
  mrb_sym method_id;
  const char *filename;
};

typedef void (*each_backtrace_func)(mrb_state*, const struct backtrace_location*, void*);

static const mrb_data_type bt_type = { "Backtrace", mrb_free };

mrb_value mrb_exc_inspect(mrb_state *mrb, mrb_value exc);
mrb_value mrb_unpack_backtrace(mrb_state *mrb, mrb_value backtrace);

#define each_backtrace_callinfo(mrb, ci_var, irep_var, pc_var, code) do {     \
  for (ptrdiff_t i__ = (mrb)->c->ci - (mrb)->c->cibase; 0 <= i__; --i__) {    \
    const mrb_callinfo *ci_var = &(mrb)->c->cibase[i__];                      \
    if (!backtrace_callinfo_p(ci_var)) continue;                              \
    const mrb_irep *irep_var = ci->proc->body.irep;                           \
    uint32_t pc_var = (uint32_t)(&ci_var->pc[-1] - irep_var->iseq);           \
    code;                                                                     \
  }                                                                           \
} while (0)

static mrb_bool
backtrace_callinfo_p(const mrb_callinfo *ci)
{
  return ci->proc
         && !MRB_PROC_CFUNC_P(ci->proc)
         && ci->proc->body.irep
         && ci->pc;
}

#ifndef MRB_NO_STDIO

static void
print_backtrace(mrb_state *mrb, struct RObject *exc, mrb_value backtrace)
{
  mrb_int i;
  mrb_int n = RARRAY_LEN(backtrace);
  mrb_value *loc, mesg;
  FILE *stream = stderr;

  if (n != 0) {
    fprintf(stream, "trace (most recent call last):\n");
    for (i=n-1,loc=&RARRAY_PTR(backtrace)[i]; i>0; i--,loc--) {
      if (mrb_string_p(*loc)) {
        fprintf(stream, "\t[%d] %.*s\n",
                (int)i, (int)RSTRING_LEN(*loc), RSTRING_PTR(*loc));
      }
    }
    if (mrb_string_p(*loc)) {
      fprintf(stream, "%.*s: ", (int)RSTRING_LEN(*loc), RSTRING_PTR(*loc));
    }
  }
  mesg = mrb_exc_inspect(mrb, mrb_obj_value(exc));
  fprintf(stream, "%.*s\n", (int)RSTRING_LEN(mesg), RSTRING_PTR(mesg));
}

/* mrb_print_backtrace

   function to retrieve backtrace information from the last exception.
*/

MRB_API void
mrb_print_backtrace(mrb_state *mrb)
{
  mrb_value backtrace;

  if (!mrb->exc) {
    return;
  }

  backtrace = mrb_obj_iv_get(mrb, mrb->exc, MRB_SYM(backtrace));
  if (mrb_nil_p(backtrace)) return;
  if (!mrb_array_p(backtrace)) backtrace = mrb_unpack_backtrace(mrb, backtrace);
  print_backtrace(mrb, mrb->exc, backtrace);
}
#else

MRB_API void
mrb_print_backtrace(mrb_state *mrb)
{
}

#endif

static mrb_value
packed_backtrace(mrb_state *mrb)
{
  uint32_t len = 0;
  each_backtrace_callinfo(mrb, ci, irep, pc, {(void)pc; ++len;});
  uint32_t size = len * sizeof(struct backtrace_location);
  struct backtrace_location *locs = (struct backtrace_location*)mrb_malloc(mrb, size);
  struct RData *backtrace = mrb_data_object_alloc(mrb, NULL, locs, &bt_type);
  backtrace->flags = len;
  each_backtrace_callinfo(mrb, ci, irep, pc, {
    locs->lineno = mrb_debug_get_line(mrb, irep, pc);
    locs->filename = mrb_debug_get_filename(mrb, irep, pc);
    locs->method_id = ci->mid;
    ++locs;
  });
  return mrb_obj_value(backtrace);
}

void
mrb_keep_backtrace(mrb_state *mrb, mrb_value exc)
{
  mrb_sym sym = MRB_SYM(backtrace);
  mrb_value backtrace;
  int ai;

  if (mrb_iv_defined(mrb, exc, sym)) return;
  ai = mrb_gc_arena_save(mrb);
  backtrace = packed_backtrace(mrb);
  mrb_iv_set(mrb, exc, sym, backtrace);
  mrb_gc_arena_restore(mrb, ai);
}

mrb_value
mrb_unpack_backtrace(mrb_state *mrb, mrb_value backtrace)
{
  const struct backtrace_location *bt;
  mrb_int n, i;
  int ai;

  if (mrb_nil_p(backtrace)) {
  empty_backtrace:
    return mrb_ary_new_capa(mrb, 0);
  }
  if (mrb_array_p(backtrace)) return backtrace;
  bt = (struct backtrace_location*)mrb_data_check_get_ptr(mrb, backtrace, &bt_type);
  if (bt == NULL) goto empty_backtrace;
  n = (mrb_int)RDATA(backtrace)->flags;
  if (n == 0) goto empty_backtrace;
  backtrace = mrb_ary_new_capa(mrb, n);
  ai = mrb_gc_arena_save(mrb);
  for (i = 0; i < n; i++) {
    const struct backtrace_location *entry = &bt[i];
    const char *filename = "(unknown)";
    int lineno = 0;
    if (entry->lineno != -1) {  /* debug info was available */
      filename = entry->filename;
      lineno = entry->lineno;
    }
    mrb_value btline = mrb_format(mrb, "%s:%d", filename, lineno);
    if (entry->method_id != 0) {
      mrb_str_cat_lit(mrb, btline, ":in ");
      mrb_str_cat_cstr(mrb, btline, mrb_sym_name(mrb, entry->method_id));
    }
    mrb_ary_push(mrb, backtrace, btline);
    mrb_gc_arena_restore(mrb, ai);
  }

  return backtrace;
}

mrb_value
mrb_exc_backtrace(mrb_state *mrb, mrb_value exc)
{
  mrb_value backtrace;

  backtrace = mrb_iv_get(mrb, exc, MRB_SYM(backtrace));
  if (mrb_nil_p(backtrace) || mrb_array_p(backtrace)) {
    return backtrace;
  }
  /* unpack packed-backtrace */
  backtrace = mrb_unpack_backtrace(mrb, backtrace);
  mrb_iv_set(mrb, exc, MRB_SYM(backtrace), backtrace);
  return backtrace;
}

mrb_value
mrb_get_backtrace(mrb_state *mrb)
{
  return mrb_unpack_backtrace(mrb, packed_backtrace(mrb));
}

const char *
mrb_get_current_filename(mrb_state *mrb)
{
  each_backtrace_callinfo(mrb, ci, irep, pc, {
    const char *f = mrb_debug_get_filename(mrb, irep, pc);
    return f;
  });
  return NULL;
}
