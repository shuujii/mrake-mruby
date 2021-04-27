## Next release (2021-??-??)

### New Features

* Presym macros support global/special variables (cd0f9406)
* Add `Z` specifier of `mrb_get_args` for `String` that is C string (d43d417e)
* Add `MRBC_CONTEXT_INITIALIZER` and `mrbc_context_finalize` (FIXME)

### Other Improvements

* Reduce memory usage of instance variable tables (3ea62295)
* Change type of `mrb_value` from struct to `uintptr_t` (c8291135)
* Reduce memory usage of method tables (98afff68)
* Improve `File.expand_path` performance (a94bbe12)

### Backwards Compatibility

* Drop support for some configurations (98afff68)
* Drop support for Windows (58c8c396)

### Bug Fixes

### Merge From Upstream mruby
