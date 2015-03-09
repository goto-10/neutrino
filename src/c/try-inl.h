//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// Try-finally macros.


#ifndef _TRY_INL
#define _TRY_INL

#include "value-inl.h"

// Plain flavor, works with normal values.
#define P_FLAVOR(F) F(value_t, is_condition, is_heap_exhausted_condition)
#define P_RETURN(V) return (V)

// Safe flavor, works with gc-safe values.
#define S_FLAVOR(F) F(safe_value_t, safe_value_is_condition, 0)

// We never return safe values, always unsafe ones. It's the caller's
// responsibility to wrap the returned value if it needs to be safe.
#define E_S_RETURN(V) E_RETURN(deref_immediate(V))

// Extract components from the types.
#define __TRY_GET_TYPE__(TYPE, IS_COND, IS_EXHAUSTED) TYPE
#define __TRY_GET_IS_COND__(TYPE, IS_COND, IS_EXHAUSTED) IS_COND
#define __TRY_GET_IS_EXHAUSTED__(TYPE, IS_COND, IS_EXHAUSTED) IS_EXHAUSTED

#define __GENERIC_TRY__(FLAVOR, EXPR, RETURN) do {                             \
  FLAVOR(__TRY_GET_TYPE__) __result__ = (EXPR);                                \
  if (FLAVOR(__TRY_GET_IS_COND__)(__result__))                                 \
    RETURN(__result__);                                                        \
} while (false)

#define __GENERIC_TRY_SET__(FLAVOR, TARGET, EXPR, RETURN) do {                 \
  FLAVOR(__TRY_GET_TYPE__) __value__ = (EXPR);                                 \
  __GENERIC_TRY__(FLAVOR, __value__, RETURN);                                  \
  TARGET = __value__;                                                          \
} while (false)

#define __GENERIC_TRY_DEF__(FLAVOR, TARGET, EXPR, RETURN)                      \
FLAVOR(__TRY_GET_TYPE__) TARGET;                                               \
__GENERIC_TRY_SET__(FLAVOR, TARGET, EXPR, RETURN)

// Try performing the given expression. If it fails with heap exhausted gc and
// try again. If this fails too abort with an oom. Otherwise store the result
// in the target variable.
#define __GENERIC_RETRY_SET__(FLAVOR, RUNTIME, TARGET, EXPR, RETURN) do {      \
  FLAVOR(__TRY_GET_TYPE__) __value__ = (EXPR);                                 \
  if (FLAVOR(__TRY_GET_IS_EXHAUSTED__)(__value__)) {                           \
    __GENERIC_TRY_DEF__(FLAVOR, __recall__,                                    \
        runtime_prepare_retry_after_heap_exhausted(RUNTIME, __value__),        \
        RETURN);                                                               \
    __value__ = (EXPR);                                                        \
    runtime_complete_retry_after_heap_exhausted(RUNTIME, __recall__);          \
    if (FLAVOR(__TRY_GET_IS_EXHAUSTED__)(__value__))                           \
      RETURN(new_out_of_memory_condition(__value__));                          \
  }                                                                            \
  __GENERIC_TRY__(FLAVOR, __value__, RETURN);                                  \
  TARGET = __value__;                                                          \
} while (false)

// Same as __GENERIC_RETRY_SET__ but declares the variable too.
#define __GENERIC_RETRY_DEF__(FLAVOR, RUNTIME, TARGET, EXPR, RETURN)           \
  FLAVOR(__TRY_GET_TYPE__) TARGET;                                             \
  __GENERIC_RETRY_SET__(FLAVOR, RUNTIME, TARGET, EXPR, RETURN)


// --- P l a i n ---

// Evaluates the given expression; if it yields a condition returns it otherwise
// ignores the value.
#define TRY(EXPR) __GENERIC_TRY__(P_FLAVOR, EXPR, P_RETURN)

// Evaluates the value and if it yields a condition bails out, otherwise assigns
// the result to the given target.
#define TRY_SET(TARGET, EXPR) __GENERIC_TRY_SET__(P_FLAVOR, TARGET, EXPR, P_RETURN)

// Declares a new variable to have the specified value. If the initializer
// yields a condition we bail out and return that value.
#define TRY_DEF(NAME, EXPR) __GENERIC_TRY_DEF__(P_FLAVOR, NAME, EXPR, P_RETURN)


// --- E n s u r e ---

// Crude exception handling. The E_ macros work like the plain ones but can
// ensure cleanup on conditions.

// Declares that the following code uses the E_ macros to perform cleanup on
// leaving the method. This is super crude and need refinement but it works okay.
#define TRY_FINALLY {                                                          \
  value_t __try_finally_result__;

// Marks the end of the try-part of a try/finally block, the following code is
// the finally part.
#define FINALLY                                                                \
  finally: {                                                                   \

// Marks the end of a try/finally block. This must come at the and of a function,
// any code following this macro will not be executed.
#define YRT                                                                    \
    }                                                                          \
    return __try_finally_result__;                                             \
  }

// Does the same as a normal return of the specified value but ensures that the
// containing function's FINALLY clause is executed.
#define E_RETURN(V) do {                                                       \
  __try_finally_result__ = (V);                                                \
  goto finally;                                                                \
} while (false)

// Does the same as a TRY except makes sure that the function's FINALLY block
// is executed before bailing out on a condition.
#define E_TRY(EXPR) __GENERIC_TRY__(P_FLAVOR, EXPR, E_RETURN)

// Does the same as TRY_SET except makes sure the function's FINALLY block is
// executed before bailing on a condition.
#define E_TRY_SET(TARGET, EXPR) __GENERIC_TRY_SET__(P_FLAVOR, TARGET, EXPR, E_RETURN)

// Does the same as TRY_DEF except makes sure the function's FINALLY block is
// executed before bailing on a condition.
#define E_TRY_DEF(NAME, EXPR) __GENERIC_TRY_DEF__(P_FLAVOR, NAME, EXPR, E_RETURN)

// Does the same as a S_TRY except makes sure that the function's FINALLY block
// is executed before bailing out on a condition.
#define E_S_TRY(EXPR) __GENERIC_TRY__(S_FLAVOR, EXPR, E_S_RETURN)

// Does the same as S_TRY_SET except makes sure the function's FINALLY block is
// executed before bailing on a condition.
#define E_S_TRY_SET(TARGET, EXPR) __GENERIC_TRY_SET__(S_FLAVOR, TARGET, EXPR, E_S_RETURN)

// Does the same as S_TRY_DEF except makes sure the function's FINALLY block is
// executed before bailing on a condition.
#define E_S_TRY_DEF(NAME, EXPR) __GENERIC_TRY_DEF__(S_FLAVOR, NAME, EXPR, E_S_RETURN)

#endif // _TRY_INL
