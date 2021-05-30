#include <stdio.h>
#include <sys/stat.h>
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/compile.h>
#include <mruby/error.h>
#include <mruby/path.h>
#include <mruby/proc.h>
#include <mruby/string.h>
#include <mruby/thash.h>
#include <mruby/variable.h>
#include <mruby/presym.h>

#define str_hash_code(str) mrb_str_hash(NULL, str)
#define str_equal_p(str1, str2) mrb_str_equal(NULL, str1, str2)

/* Set of String object */
MRB_TSET_INIT(mrb_strset, mrb_value, mrb_nil_value(), mrb_false_value(),
              mrb_test, str_hash_code, str_equal_p)

#define EXT_RB ".rb"
#define ext_rb_p(ext) (strcmp(ext, EXT_RB) == 0)

#define each_path_in_load_path(mrb, relative_path, path_var, code) do {       \
  mrb_value load_path_ary__ = get_load_paths(mrb), path_var;                  \
  mrb_value *load_paths__ = RARRAY_PTR(load_path_ary__);                      \
  mrb_int load_paths_len__ = RARRAY_LEN(load_path_ary__);                     \
  int ai__ = mrb_gc_arena_save(mrb), i__= 0;                                  \
  for (; mrb_gc_arena_restore(mrb, ai__), i__ < load_paths_len__; ++i__) {    \
    mrb_string_cstr_str(mrb, load_paths__[i__]);                              \
    path_var = mrb_path_expand(mrb, relative_path, load_paths__[i__]);        \
    code;                                                                     \
  }                                                                           \
} while (0)

static mrb_bool
loadable_file_p(mrb_value path)
{
  struct stat st;
  if (stat(RSTRING_PTR(path), &st) != 0) return FALSE;
  mode_t m = st.st_mode;
  return (m & S_IRUSR) && (S_ISREG(m) || S_ISFIFO(m) || S_ISCHR(m));
}

static void
load_rb_file(mrb_state *mrb, mrb_value path)
{
  int ai = mrb_gc_arena_save(mrb);
  const char *p = RSTRING_PTR(path);
  FILE *fp = fopen(p, "r");
  if (!fp) mrb_sys_fail(mrb, "fopen");
  mrbc_context c[] = {MRBC_CONTEXT_INITIALIZER};
  mrbc_filename(mrb, c, p);
  mrb_load_file_cxt(mrb, fp, c);
  fclose(fp);
  mrbc_context_finalize(mrb, c);
  struct REnv *env = mrb_vm_ci_env(mrb->c->cibase);
  mrb_vm_ci_env_set(mrb->c->cibase, NULL);
  mrb_env_unshare(mrb, env);
  mrb_gc_arena_restore(mrb, ai);
}

static mrb_value
get_load_paths(mrb_state *mrb)
{
  mrb_value load_paths = mrb_gv_get(mrb, MRB_GVSYM(LOAD_PATH));
  if (!mrb_array_p(load_paths)) {
    mrb_raise(mrb, E_TYPE_ERROR, "$LOAD_PATH must be Array");
  }
  return load_paths;
}

static mrb_noreturn void
load_error(mrb_state *mrb, mrb_value path)
{
  mrb_value m;
  if (mrb_nil_p(path)) {
     m = mrb_str_new_lit(mrb, "cannot infer basepath");
  } else {
     m = mrb_str_new_lit(mrb, "cannot load such file -- ");
     mrb_str_cat_str(mrb, m, path);
  }
  mrb_value e = mrb_exc_new_str(mrb, mrb_exc_get_id(mrb, MRB_SYM(LoadError)), m);
  if (!mrb_nil_p(path)) mrb_obj_iv_set(mrb, mrb_obj_ptr(e), MRB_SYM(path), path);
  mrb_exc_raise(mrb, e);
}

static int
required_features_mark_i(mrb_state *mrb, mrb_strset_iter it, void *data)
{
  mrb_gc_mark_value(mrb, mrb_strset_it_key(it));
  return 0;
}

static void
required_features_mark(mrb_state *mrb)
{
  mrb_strset_each(mrb, mrb->required_feature_paths, required_features_mark_i, NULL);
  mrb_strset_each(mrb, mrb->required_feature_names, required_features_mark_i, NULL);
}

static mrb_bool
required_feature_path_p(mrb_state *mrb, mrb_value path)
{
  return mrb_strset_include_p(mrb->required_feature_paths, path);
}

static mrb_bool
required_feature_name_p(mrb_state *mrb, mrb_value feature)
{
  return mrb_strset_include_p(mrb->required_feature_names, feature);
}

static mrb_bool
require_rb_file(mrb_state *mrb, mrb_value path)
{
  mrb_strset_add(mrb, &mrb->required_feature_paths, path);
  load_rb_file(mrb, path);
  return TRUE;
}

static mrb_value
lde_path(mrb_state *mrb, mrb_value self)
{
  return mrb_obj_iv_get(mrb, mrb_obj_ptr(self), MRB_SYM(path));
}

static void
mrb_load(mrb_state *mrb, mrb_value fname)
{
  mrb_assert(mrb_str_cstr_p(fname));
  const char *f = RSTRING_PTR(fname);
  if (mrb_pathz_implicit_relative_p(f)) {
    each_path_in_load_path(mrb, fname, path, {
      if (loadable_file_p(path)) return load_rb_file(mrb, path);
    });
    if (loadable_file_p(fname)) return load_rb_file(mrb, fname);
  } else {
    mrb_value path = mrb_path_expand(mrb, fname, mrb_nil_value());
    if (loadable_file_p(path)) return load_rb_file(mrb, path);
  }
  load_error(mrb, fname);
}

static mrb_bool
mrb_require(mrb_state *mrb, mrb_value feature)
{
  mrb_assert(mrb_str_cstr_p(feature));
  const char *f = RSTRING_PTR(feature);
  mrb_int ext_len;
  if (mrb_pathz_implicit_relative_p(f)) {
    if (required_feature_name_p(mrb, feature)) return FALSE;
    const char *ext = mrb_path_extname(feature, &ext_len);
    mrb_bool needs_ext = !ext_rb_p(ext);
    each_path_in_load_path(mrb, feature, path, {
      if (needs_ext) mrb_str_cat_lit(mrb, path, EXT_RB);
      if (required_feature_path_p(mrb, path)) return FALSE;
      if (!loadable_file_p(path)) continue;
      mrb_strset_add(mrb, &mrb->required_feature_names, feature);
      return require_rb_file(mrb, path);
    });
  } else {
    mrb_value path = mrb_path_expand(mrb, feature, mrb_nil_value());
    const char *ext = mrb_path_extname(path, &ext_len);
    if (!ext_rb_p(ext)) mrb_str_cat_lit(mrb, path, EXT_RB);
    if (required_feature_path_p(mrb, path)) return FALSE;
    if (loadable_file_p(path)) return require_rb_file(mrb, path);
  }
  load_error(mrb, feature);
  return FALSE;  /* not reached */
}

static mrb_value
krn_load(mrb_state *mrb, mrb_value self)
{
  mrb_value fname = mrb_string_cstr_str(mrb, mrb_get_arg1(mrb));
  mrb_load(mrb, fname);
  return mrb_true_value();
}

static mrb_value
krn_require(mrb_state *mrb, mrb_value self)
{
  mrb_value feature = mrb_string_cstr_str(mrb, mrb_get_arg1(mrb));
  return mrb_bool_value(mrb_require(mrb, feature));
}

void
mrb_mruby_require_gem_init(mrb_state *mrb)
{
  struct RClass *krn = mrb->kernel_module;
  mrb_define_method(mrb, krn, "load", krn_load, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, krn, "require", krn_require, MRB_ARGS_REQ(1));

  struct RClass *sce = mrb_exc_get_id(mrb, MRB_SYM(ScriptError));
  struct RClass *lde = mrb_define_class_id(mrb, MRB_SYM(LoadError), sce);
  mrb_define_method(mrb, lde, "path", lde_path, MRB_ARGS_NONE());

  mrb_gv_set(mrb, MRB_GVSYM(LOAD_PATH), mrb_ary_new(mrb));

  mrb->required_features_mark = required_features_mark;
}

void
mrb_mruby_require_gem_final(mrb_state *mrb)
{
  mrb_strset_free(mrb, mrb->required_feature_paths);
  mrb_strset_free(mrb, mrb->required_feature_names);
}
