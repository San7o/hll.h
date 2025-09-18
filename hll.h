//////////////////////////////////////////////////////////////////////
// SPDX-License-Identifier: MIT
//
// hll.h
// -----
//
// Configurable, header-only implementation of HyperLogLog in C99.
//
// Author: Giovanni Santini
// Mail: giovanni.santini@proton.me
// License: MIT
//
//
// Documentation
// -------------
//
// This header-only C99 library implements the HyperLogLog algorithm
// for approximating the cardinality (number of distinct elements) of
// large multisets.
//
// The algorithm uses a hash function to map input elements to uniformly
// distributed bit patterns and records the maximum run of leading zeros
// across many independent registers. From this, it computes an estimate
// of the set’s cardinality with low memory footprint and tunable accuracy.
//
// Features:
//   - Header-only, portable C99 implementation
//   - Adjustable precision/space-accuracy tradeoff
//   - Suitable for large-scale data streams
//
// Reference:
//   S. Heule, M. Nunkesser, A. Hall,
//   "HyperLogLog in Practice: Algorithmic Engineering of a State of
//    The Art Cardinality Estimation Algorithm"
//
//
// Usage
// -----
//
// Do this:
//
//   #define HLL_IMPLEMENTATION
//
// before you include this file in *one* C or C++ file to create the
// implementation.
//
// i.e. it should look like this:
//
//   #include ...
//   #include ...
//   #include ...
//   #define HLL_IMPLEMENTATION
//   #include "hll.h"
//
// You can tune the library by #defining certain values. See the
// "Config" comments under "Configuration" below.
//
// You need to initialize a hll_t with `hll_init` and eventually
// destroy it with `hll_destroy`. To add a value, use `hll_add`. To
// get an estimate of the cardinality of the values, use
// `hll_count`. See the function definitions for more information.
//
// Check some examples at the end of the header.
//
//
// TODO
// ----
//
// The library should be tested more. Right now, some precision values
// do not work well so the optimal precision should be handpicked
// manually.
// Only the dense representation is implemented.
// Additionally, no empirical bias correction is applied.
//

#ifndef _HLL_H_
#define _HLL_H_

#define HLL_VERSION_MAJOR 0
#define HLL_VERSION_MINOR 1

#ifdef __cplusplus
extern "C" {
#endif

//
// Configurations
//

// Config: the default precision.
//
// Note: Must be in range [HLL_PRECISION_MIN..HLL_PRECISION_MAX]
#ifndef HLL_PRECISION
  #define HLL_PRECISION 10
#endif

// Config: element type that can be added in hll
#ifndef HLL_ELEMENT_T
  #define HLL_ELEMENT_T char*
#endif

// Config: hash type
#ifndef HLL_HASH_T
  #define HLL_HASH_T unsigned int
#endif

// Config: the hash input value
#ifndef HLL_HASH_INPUT_T
#define HLL_HASH_INPUT_T HLL_ELEMENT_T
#endif

// Config: The default hash function
#ifndef HLL_HASH_FUNC
  #define HLL_HASH_FUNC hll_hash_string
#endif

// Config: The allocator function.
//
// Note: Should behave like calloc(3) and set the memory to 0
#ifndef HLL_CALLOC
  #include <stdlib.h>
  #define HLL_CALLOC calloc
#endif

// Config: The function to free allocated memory
//
// Note: Should behave like free(3)
#ifndef HLL_FREE
  #include <stdlib.h>
  #define HLL_FREE free
#endif

// Config: Base two floating point log function.
//
// Note: Should be used like logf(3).
#ifndef HLL_LOGF
  #include <math.h>
  #define HLL_LOGF logf
#endif

// Config: floating point power function
//
// Note: should be used like powf(3)
#ifndef HLL_POWF
  #include <math.h>
  #define HLL_POWF powf
#endif

//
// Types
//

#define HLL_PRECISION_MIN 4
#define HLL_PRECISION_MAX 16

typedef HLL_ELEMENT_T hll_element_t;
typedef HLL_HASH_T hll_hash_t;
typedef HLL_HASH_INPUT_T hll_hash_input_t;

// Type of an hash function
//
// Args:
//  - arg1: the input of the hash function
//  - arg2: the size of the input
//
// Returns: the hash value of the input
typedef hll_hash_t (*hll_hash_func_t)(hll_hash_input_t, unsigned int);

// HyperLogLog
typedef struct {
  // _registers[i] stores the maximum number of leading zero plus one
  // for substream with index i
  //
  // The input stream of data element is divided into m substreams
  // using the first [precision] bits of the hash values, where m =
  // 2^[precision].
  hll_hash_t *_registers;
  // Number of bits used to calculate the substream value of an input
  // stream. Higher number means more substreams and more precision,
  // but requires more memory.
  //
  // Note: Must be a number in range [HLL_PRECISION_MIN..HLL_PRECISION_MAX]
  unsigned int precision;
  // The hash function
  hll_hash_func_t hash;
} hll_t;

//
// Errors
//

typedef int hll_error;
#define HLL_OK                       0
#define HLL_ERROR_HLL_NULL          -1
#define HLL_ERROR_INVALID_PRECISION -2
#define HLL_ERROR_HLL_UNINITIALIZED -3
#define HLL_ERROR_ALLOCATING_MEMORY -4
#define _HLL_ERROR_MAX              -5

//
// Function Definitions
//

// Initialize hll with fields
//
// Args:
//  - arg1: a pointer to the hll to initialize
//  - args...: hll fields
//
// Returns: 0 on success, or a negative hll_error
//
// Notes: Allocates memory with HLL_CALLOC. You should call
// hll_destroy when you are done.
//
// Example:
// hll_t my_hll;
// hll_init(&my_hll, .precision = 10);
#define hll_init(hll, ...) _hll_init_impl(      \
  hll,                                          \
  &(hll_t) {                                    \
    .precision = HLL_PRECISION,                 \
    .hash = HLL_HASH_FUNC,                      \
    __VA_ARGS__,                                \
  })

// Initialize hll from settings
//
// Args:
//  - hll: a pointer to the hll to initialize
//  - hll_src: fields will be copied from here
//
// Returns: 0 on success, or a negative hll_error
//
// Notes: Allocates memory with HLL_CALLOC. You should call
// hll_destroy when you are done.
hll_error _hll_init_impl(hll_t *hll, hll_t *hll_src);

// Initialize hll
//
// Args:
//  - precision: a number between HLL_PRECISION_MIN and HLL_PRECISION_MAX
//  - hash: the hash function
//
// Returns: 0 on success, or a negative hll_error
//
// Notes: Allocates memory with HLL_CALLOC. You should call
// hll_destroy when you are done.
hll_error hll_init2(hll_t *hll,
                   unsigned int precision);

// Destroy an hll
//
// Args:
//  - hll: pointer to the hll to delete
//
// Returns: 0 on success, or a negative hll_error
//
// Notes: uses HLL_FREE. Must be called after hll_init or hll_init2
hll_error hll_destroy(hll_t *hll);

// Add an element to the hll structure
//
// Args:
//  - hll: pointer to the hll struct
//  - element: element to insert
//  - element_len: length of the element
//
// Returns: 0 on success, or a negative hll_error
hll_error hll_add(hll_t *hll,
                  hll_element_t element,
                  unsigned int element_len);

// Get an estimate of the cardinality of the elements
//
// Args:
//   - hll: pointer to a hll structure
//
// Returns: a non-negative estimation of the cardinality, or a
// negative hll_error
int hll_count(hll_t *hll);

// Merge hll stc into hll destination
//
// Args:
//  - hll_dest: pointer to the destination hll structure
//  - hll_src: pointer to the source hll structure
hll_error hll_merge(hll_t *hll_dest, hll_t *hll_src);

// Fast hash function for strings
//
// Args:
//  - input: hash input
//  - input_len: length of the input
//
// Returns: the hashed value of the input
unsigned int hll_hash_string(char* input, unsigned int input_len);

// Convert error value into string
//
// Args:
//  - error: the (negative) error valie
//
// Returns: a string describing the error
const char *hll_error_string(hll_error error);

//
// Implementation
//

#ifdef HLL_IMPLEMENTATION

hll_error _hll_init_impl(hll_t *hll, hll_t *hll_src)
{
  if (hll == NULL)
    return HLL_ERROR_HLL_NULL;

  if (hll_src != NULL)
    *hll = *hll_src;

  if (hll->precision < HLL_PRECISION_MIN
      || hll->precision > HLL_PRECISION_MAX)
    return HLL_ERROR_INVALID_PRECISION;
  
  hll->_registers = HLL_CALLOC(sizeof(hll_hash_t), (1<<hll->precision));
  if (hll->_registers == NULL)
    return HLL_ERROR_ALLOCATING_MEMORY;
  
  return HLL_OK;
}

hll_error hll_init2(hll_t *hll,
                   unsigned int precision)
{
  return _hll_init_impl(hll, &(hll_t){.precision = precision});
}

hll_error hll_destroy(hll_t *hll)
{
  if (hll == NULL)
    return HLL_ERROR_HLL_NULL;

  if (hll->_registers == NULL)
    return HLL_ERROR_HLL_UNINITIALIZED;

  HLL_FREE(hll->_registers);
  
  return HLL_OK;
}

hll_hash_t hll_max(hll_hash_t a, hll_hash_t b)
{
  return (a > b) ? a : b;
}

unsigned int hll_get_hash_zeros(hll_hash_t hash, unsigned int precision)
{
  hll_hash_t head = hash & ((1 << precision) - 1);
  unsigned int count = 0;
  for (unsigned int i = 0; i < sizeof(hll_hash_t)*8; ++i)
  {
    if (head & (1<<i))
      break;
    count++;
  }
  return count;
}
  
hll_error hll_add(hll_t *hll,
                  hll_element_t element,
                  unsigned int element_len)
{
  if (hll == NULL)
    return HLL_ERROR_HLL_NULL;

  if (hll->_registers == NULL)
    return HLL_ERROR_HLL_UNINITIALIZED;

  // Read the paper to understand what is happening
  hll_hash_t hashed_elem = hll->hash(element, element_len);
  unsigned int offset    = sizeof(hll_hash_t)*8 - hll->precision;
  unsigned int mask      = ~((1 << offset) - 1);
  hll_hash_t idx         = (hashed_elem & mask) >> offset;
  hll_hash_t hash_zeros  = hll_get_hash_zeros(hashed_elem, hll->precision);
  hll->_registers[idx]   = hll_max(hll->_registers[idx], hash_zeros + 1);
  
  return HLL_OK;
}

int hll_count(hll_t *hll)
{
  if (hll == NULL)
    return HLL_ERROR_HLL_NULL;

  if (hll->_registers == NULL)
    return HLL_ERROR_HLL_UNINITIALIZED;

  // Read the paper to understand what is happening
  float magic = 0.0f;
  unsigned int registers_len = (1 << hll->precision);
  switch(registers_len)
  {
  case 16:
    magic = 0.673;
    break;
  case 32:
    magic = 0.697;
    break;
  case 64:
    magic = 0.709;
    break;
  default:
    magic = 0.7213 / (1 + 1.079 / (registers_len));
    break;
  }
  
  float sum = 0.0f;
  for (unsigned int i = 0; i < registers_len - 1; ++i)
    sum += HLL_POWF(2, -hll->_registers[i]);

  float estimate = magic * registers_len * registers_len * (1 / sum);
  if (estimate < 5 / 2 * registers_len)
  {
    unsigned int num_of_zero_registers = 0;
    for (unsigned int i = 0; i < registers_len; ++i)
      if (hll->_registers[i] == 0)
        num_of_zero_registers++;

    if (num_of_zero_registers == 0)
      return estimate;

    // Linear counting
    return registers_len * HLL_LOGF(registers_len / num_of_zero_registers);
  } else if (estimate <= (float)((1ULL << 32) / 30))
  {
    return estimate;
  } else {
    return -(float)(1ULL << 32) * HLL_LOGF(1 - estimate / (float)(1ULL << 32));
  }
  
  return 0;
}

hll_error hll_merge(hll_t *hll_dest, hll_t *hll_src)
{
  if (hll_dest == NULL || hll_src == NULL)
    return HLL_ERROR_HLL_NULL;

  if (hll_dest->_registers == NULL || hll_src->_registers == NULL)
    return HLL_ERROR_HLL_UNINITIALIZED;

  for (int i = 0;
       i < (1 << hll_dest->precision)
         && i < (1 << hll_src->precision);
       ++i)
    hll_dest->_registers[i] = hll_max(hll_dest->_registers[i],
                                      hll_src->_registers[i]);
  
  return HLL_OK;
}

// hash [bytes] of size [len]
// Credits to http://www.cse.yorku.ca/~oz/hash.html
unsigned int hll_hash_string(char *bytes, unsigned int len)
{
  unsigned int hash = 5381;
  for (unsigned int i = 0; i < len; ++i)
    hash = hash * 33 + bytes[i];
  return hash;
}

#if _HLL_ERROR_MAX != -5
  #error "Updated HLL_ERRORs, should update hll_error_string"
#endif
const char *hll_error_string(hll_error error)
{
  if (error >= 0)
    return "HLL_OK";
  
  switch(error)
  {
  case HLL_ERROR_HLL_NULL:
    return "HLL_ERROR_HLL_NULL";
  case HLL_ERROR_INVALID_PRECISION:
    return "HLL_ERROR_INVALID_PRECISION";
  case HLL_ERROR_HLL_UNINITIALIZED:
    return "HLL_ERROR_HLL_UNINITIALIZED";
  case HLL_ERROR_ALLOCATING_MEMORY:
    return "HLL_ERROR_ALLOCATING_MEMORY";
  default:
    break;
  }
  
  return "HLL_ERROR_UNKNOWN";
}

#endif // HLL_IMPLEMENTATION

//
// Examples
//

#if 0
#include <assert.h>
#include <stdio.h>

// LCG pseudo random number generator
#define MAGIC1 1664525    // a
#define MAGIC2 1013904223 // c
#define MAGIC3 (1<<31)    // m
unsigned int lcg(unsigned int seed)
{
  return (MAGIC1 * seed + MAGIC2) % MAGIC3;
}

// 4-byte integer hashing
// https://burtleburtle.net/bob/hash/integer.html
unsigned int integer_hash(unsigned int a, unsigned int _b)
{
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}

#define HLL_IMPLEMENTATION
#define HLL_ELEMENT_T unsigned int
#define HLL_HASH_FUNC integer_hash
#include "hll.h"

#define MAX_NUMBER 5000
#define MAX_ITERATIONS 3000

int main(void)
{
  hll_t hll;
  assert(hll_init(&hll, .precision = 9) == HLL_OK);

  _Bool unique_numbers[MAX_NUMBER] = {0};
  unsigned int seed = 6969;
  unsigned int random_value = lcg(seed);
  for (int i = 0; i < MAX_ITERATIONS; ++i)
  {
    unique_numbers[random_value % MAX_NUMBER] = 1;
    assert(hll_add(&hll, random_value % MAX_NUMBER, 4) == HLL_OK);
    random_value = lcg(random_value);
  }

  int expected = 0;
  for (int i = 0; i < MAX_NUMBER; ++i)
    if (unique_numbers[i])
      expected++;

  int estimate = hll_count(&hll);
  assert(estimate >= 0);

  printf("Expected: %d\n", expected);
  printf("Estimate: %d\n", estimate);
  
  assert(hll_destroy(&hll) == HLL_OK);
  return 0;
}
#endif // 0

#ifdef __cplusplus
}
#endif
  
#endif // _HLL_H_
