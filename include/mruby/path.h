#ifndef MRUBY_PATH_H
#define MRUBY_PATH_H

/*
 * File path utilities
 */

#include "common.h"

/*
 * `z` suffix (e.g. `pathz`) means to reveive C string.
 */

MRB_BEGIN_DECL

mrb_bool mrb_pathz_explicit_relative_p(const char *path);
char *mrb_path_basename(mrb_value path, mrb_int *lenp);
char *mrb_path_extname(mrb_value path, mrb_int *lenp);
mrb_value mrb_path_current_user_home(mrb_state *mrb, mrb_value out);
mrb_value mrb_path_current_dir(mrb_state *mrb, mrb_value out);
mrb_value mrb_path_expand(mrb_state *mrb, mrb_value fname, mrb_value dname);

MRB_INLINE mrb_bool
mrb_file_sep_p(int c)
{
  return c == '/';
}

MRB_INLINE mrb_bool
mrb_pathz_absolute_p(const char *path)
{
  return mrb_file_sep_p(path[0]);
}

MRB_INLINE mrb_bool
mrb_pathz_implicit_relative_p(const char *path)
{
  return path[0] != '~'
         && !mrb_pathz_absolute_p(path)
         && !mrb_pathz_explicit_relative_p(path);
}

MRB_END_DECL

#endif  /* MRUBY_PATH_H */
