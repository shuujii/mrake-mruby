#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <mruby.h>
#include <mruby/error.h>
#include <mruby/string.h>
#include <mruby/path.h>
#include <mruby/presym.h>

static const char *
pathz_next_part(const char *p, mrb_int *lenp)
{
  const char *beg = p;
  while (*p && !mrb_file_sep_p(*p)) ++p;
  *lenp = p - beg;
  return *p ? ++p : p;
}

static mrb_value
path_expand(mrb_state *mrb, mrb_value fname, mrb_value dname, mrb_value out)
{
  const char *f = RSTRING_PTR(fname), *fend = f + RSTRING_LEN(fname);
  mrb_int capa = 0;

  /* convert to absolute path */
  if (mrb_pathz_absolute_p(f)) {
  } else if (*f == '~') {
    ++f;
    if (mrb_file_sep_p(*f) || !*f) {
      mrb_path_current_user_home(mrb, out);
    } else {
      mrb_raise(mrb, E_ARGUMENT_ERROR, "'~USER' expanding isn't unsupported");
    }
    if (!mrb_pathz_absolute_p(RSTRING_PTR(out))) {
      mrb_raise(mrb, E_ARGUMENT_ERROR, "non-absolute home");
    }
  } else { /* relative path */
    if (mrb_nil_p(dname)) {
      mrb_path_current_dir(mrb, out);
    } else {
      path_expand(mrb, dname, mrb_nil_value(), out);
    }
    ++capa;
  }

  /* convert to canonical path */
  capa += RSTRING_LEN(out) + (fend - f);
  mrb_str_resize_capa(mrb, out, capa);
  const char *pbeg;
  char *obeg = RSTRING_PTR(out), *o = obeg + RSTRING_LEN(out);
  mrb_int plen;
  while (*f) {
    pbeg = f;
    f = pathz_next_part(f, &plen);
    if (plen == 0 || (plen == 1 && pbeg[0] == '.')) {
      /* skip */
    } else if (plen == 2 && pbeg[0] == '.' && pbeg[1] == '.') {
      /* go back to the parent */
      if (o != obeg) for (--o; !mrb_file_sep_p(*o); --o);
    } else {
      *o++ = '/';
      memcpy(o, pbeg, plen);
      o += plen;
    }
  }

  mrb_int olen = o - obeg;
  if (olen == 0) obeg[olen++] = '/';
  RSTR_SET_LEN(mrb_str_ptr(out), olen);
  obeg[olen] = 0;
  return out;
}

mrb_bool
mrb_pathz_explicit_relative_p(const char *path)
{
  if (*path++ != '.') return FALSE;
  if (*path == '.') path++;
  return mrb_file_sep_p(*path);
}

mrb_value
mrb_path_current_user_home(mrb_state *mrb, mrb_value out)
{
  const char *dir = getenv("HOME");
  if (dir) {
    mrb_str_cat_cstr(mrb, out, dir);
  } else {
    mrb_raise(mrb, E_ARGUMENT_ERROR,
              "couldn't find HOME environment -- expanding '~'");
  }
  return out;
}

mrb_value
mrb_path_current_dir(mrb_state *mrb, mrb_value out)
{
  mrb_int olen = RSTRING_LEN(out), blen = RSTRING_CAPA(out) - olen + 1;
  char *b = RSTRING_PTR(out) + olen;
  while (!getcwd(b, blen)) {
    if (errno == ERANGE) {
      blen = blen * 2 - 1;
      mrb_str_resize_capa(mrb, out, olen + blen);
      b = RSTRING_PTR(out) + olen;
    } else {
      mrb_sys_fail(mrb, "getcwd");
    }
  }
  RSTR_SET_LEN(mrb_str_ptr(out), strlen(b));
  return out;
}

mrb_value
mrb_path_expand(mrb_state *mrb, mrb_value fname, mrb_value dname)
{
  mrb_string_cstr_str(mrb, fname);
  if (!mrb_nil_p(dname)) mrb_string_cstr_str(mrb, dname);
  return path_expand(mrb, fname, dname, mrb_str_new_capa(mrb, 257));
}
