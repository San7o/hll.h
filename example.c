// SPDX-License-Identifier: MIT
// Author:  Giovanni Santini
// Mail:    giovanni.santini@proton.me
// Github:  @San7o

#define HLL_IMPLEMENTATION
#define HLL_ELEMENT_T unsigned int
#define HLL_HASH_FUNC integer_hash
#include "hll.h"

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

int main(void)
{
  hll_t hll;
  assert(hll_init(&hll, .precision = 10) == HLL_OK);

  #define MAX_NUMBER 5000
  #define MAX_ITERATIONS 3000
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
