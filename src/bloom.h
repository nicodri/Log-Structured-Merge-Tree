#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

typedef uint64_t index_t;
typedef uint64_t key_t;
typedef struct {
  index_t size; // in bits
  index_t count; // in bits
  index_t *table;
} bloom_filter_t;
#define N_HASHES 3

void set_bit(bloom_filter_t *B, index_t i);
index_t get_bit(bloom_filter_t *B, index_t i);

index_t hash1(bloom_filter_t *B, key_t k);
index_t hash2(bloom_filter_t *B, key_t k);

bloom_filter_t * bloom_init(index_t size_in_bits);
void bloom_destroy(bloom_filter_t *B);
int bloom_check(bloom_filter_t *B, key_t k);
void bloom_add(bloom_filter_t *B, key_t k);
