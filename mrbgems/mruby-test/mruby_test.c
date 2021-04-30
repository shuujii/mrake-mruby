#include <stdlib.h>
#include <unistd.h>
#include <mruby.h>
#include <mruby/class.h>
#include <mruby/error.h>
#include <mruby/path.h>
#include <mruby/string.h>

static mrb_value
test_getenv(mrb_state *mrb, mrb_value klass)
{
  const char *name = mrb_string_cstr(mrb, mrb_get_arg1(mrb));
  const char *value = getenv(name);
  return value ? mrb_str_new_cstr(mrb, value) : mrb_nil_value();
}

static mrb_value
test_setenv(mrb_state *mrb, mrb_value klass)
{
  const char *namez;
  mrb_value value;
  mrb_get_args(mrb, "zZ!", &namez, &value);
  if (mrb_nil_p(value)) {
    unsetenv(namez);
  } else {
    setenv(namez, RSTRING_PTR(value), TRUE);
  }
  return value;
}

static mrb_value
test_pwd(mrb_state *mrb, mrb_value klass)
{
  return mrb_path_current_dir(mrb, mrb_str_new_capa(mrb, 255));
}

static mrb_value
test_chdir(mrb_state *mrb, mrb_value klass)
{
  const char *dir = mrb_string_cstr(mrb, mrb_get_arg1(mrb));
  if (chdir(dir) != 0) mrb_sys_fail(mrb, "chdir");
  return mrb_fixnum_value(0);
}

void
mrb_init_test_mruby_test(mrb_state *mrb)
{
  struct RClass *m = mrb_define_module(mrb, "MRubyTest");
  mrb_define_const(mrb, m, "FLOAT_TOLERANCE", mrb_float_value(mrb, 1e-10));
  mrb_define_class_method(mrb, m, "getenv", test_getenv, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, m, "setenv", test_setenv, MRB_ARGS_REQ(2));
  mrb_define_class_method(mrb, m, "pwd", test_pwd, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, m, "chdir", test_chdir, MRB_ARGS_REQ(1));
}
