#include <mruby.h>

void
mrb_init_test_mruby_test(mrb_state *mrb)
{
  struct RClass *m = mrb_define_module(mrb, "MRubyTest");
  mrb_define_const(mrb, m, "FLOAT_TOLERANCE", mrb_float_value(mrb, 1e-10));

}
