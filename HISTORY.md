## Next release (2021-??-??)

### New Features

* Presym macros support global/special variables (cd0f9406)
* Add `Z` specifier of `mrb_get_args` for `String` that is C string (d43d417e)
* Add `MRBC_CONTEXT_INITIALIZER` and `mrbc_context_finalize` (8cdb7d0e)
* Add some utility methods for test (fc3e8bf5)
* Introduce tiny hash table based map/set template based on smap (ef781a0a)
* Add `mrb_unused` (FIXME)

### Other Improvements

* Reduce memory usage of instance variable tables (3ea62295)
* Change type of `mrb_value` from struct to `uintptr_t` (c8291135)
* Reduce memory usage of method tables (98afff68)
* Improve `File.expand_path` performance (a94bbe12)
* Improve `File.extname` performance (0a73dfbe)
* Avoid making filename duplicates in `mrbc_filename` (39ec249a)

### Backwards Compatibility

* Drop support for some configurations (98afff68)
* Drop support for Windows (58c8c396)
* Exclude files in `fixtures` directories for test ruby files (c98ff54d)
* Rename `Mrbtest` to `MRubyTest` and separate file to `mruby_test.c` (4fc81344)
* Introduce `MRB_NO_INLINE_SYMBOL` instead of `MRB_USE_ALL_SYMBOLS` (dab56aef)

### Bug Fixes

### Merge From Upstream mruby
