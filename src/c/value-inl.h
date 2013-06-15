#include "value.h"

#ifndef _VALUE_INL
#define _VALUE_INL

// Evaluates the given expression; if it yields a signal returns it otherwise
// ignores the value.
#define TRY(EXPR) do {                 \
  value_t __result__ = (EXPR);         \
  if (in_domain(vdSignal, __result__)) \
    return __result__;                 \
} while (false)

// Evaluates the value and if it yields a signal bails out, otherwise assigns
// the result to the given target.
#define TRY_SET(TARGET, VALUE) do { \
  value_t __value__ = (VALUE);      \
  TRY(__value__);                   \
  TARGET = __value__;               \
} while (false)

// Declares a new variable to have the specified value. If the initializer
// yields a signal we bail out and return that value.
#define TRY_DEF(name, INIT) \
value_t name;               \
TRY_SET(name, INIT)

// Returns true if the value of the given expression is in the specified
// domain.
static inline bool in_domain(value_domain_t domain, value_t value) {
  return get_value_domain(value) == domain;
}

// Returns true iff the given value is an object within the given family.
static inline bool in_family(object_family_t family, value_t value) {
  if (!in_domain(vdObject, value))
    return false;
  return get_object_family(value) == family;
}

#endif // _VALUE_INL