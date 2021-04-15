#ifndef MRUBY_UNSUPPORTED_CONF_H
#define MRUBY_UNSUPPORTED_CONF_H

#ifdef MRB_32BIT
# error MRake does not support MRB_32BIT
#endif
#ifdef MRB_USE_FLOAT32
# error MRake does not support MRB_USE_FLOAT32
#endif
#ifdef MRB_NO_FLOAT
# error MRake does not support MRB_NO_FLOAT
#endif
#ifdef MRB_USE_METHOD_T_STRUCT
# error MRake does not support MRB_USE_METHOD_T_STRUCT
#endif
#ifdef MRB_NAN_BOXING
# error MRake does not support MRB_NAN_BOXING
#endif
#ifdef MRB_NO_BOXING
# error MRake does not support MRB_NO_BOXING
#endif
#ifdef MRB_INT32
# error MRake does not support MRB_INT32
#endif
#ifdef MRB_NO_STDIO
# error MRake does not support MRB_NO_STDIO
#endif

#endif  /* MRUBY_UNSUPPORTED_CONF_H */
