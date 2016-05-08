#include "LSMtree.h"

void set_bit(bloom_filter_t *B, index_t i){
    assert(i < B->size);

    index_t wsize = (sizeof(index_t) * 8);
    index_t position = i % wsize; 
    index_t index = i / wsize;

    B->table[index] |= (0x1UL << position); 
}

index_t get_bit(bloom_filter_t *B, index_t i){
    assert(i < B->size);

    index_t wsize = (sizeof(index_t) * 8);
    // moving the bit to 1st position
    index_t masked =(B->table[i / wsize] >> (i % wsize));

    return masked & 0x1UL;
}

// The sequence of hash functions used is a linear combination of the
// two following hashes
index_t hash1(bloom_filter_t *B, key_t_ k){
    // from https://gist.github.com/badboy/6267743

    index_t key = k;
    key = (~key) + (key << 21); // key = (key << 21) - key - 1;
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8); // key * 265
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4); // key * 21
    key = key ^ (key >> 28);
    key = key + (key << 31);
    return key;
}

index_t hash2(bloom_filter_t *B, key_t_ k){
    // suggested by Knuth in "The Art of Computer Programming"

    index_t h = (k * 11400714819323198549ul);

    return h;
}

index_t hash3(bloom_filter_t *B, key_t_ k) {
    // using FNV1-hash
    //https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
    
    key_t_ hash = 0xcbf29ce484222325;
    key_t_ fnv_prime = 0x100000001b3;

    key_t_ mask = 0xff;
    int pos = 0;
    while(mask) {
        hash = hash * fnv_prime;
        hash = hash ^ ((k & mask) >> pos);

        // shift mask by 8
        pos += 8;
        mask <<= 8;
    }

    return hash;
}

void bloom_init(bloom_filter_t * B, index_t size_in_bits, int hashes){
    // Allocate Memory
    if (B == NULL) B = (bloom_filter_t*)malloc(sizeof(bloom_filter_t));

    // Initialize attributes
    B->hashes = hashes;
    B->size = size_in_bits;
    B->count = 0;
    B->table = (index_t*)malloc(sizeof(index_t) *(size_in_bits / 8)); // convert size from bits to bytes

    if(!B->table) {
        printf("memory error\n");
    }
    // set values to zero
    memset(B->table, 0, size_in_bits / 8);
}

void bloom_destroy(bloom_filter_t *B){
    if(B) {
        if(B->table)free(B->table);
        B->table = NULL;
        free(B);
    }
}

int bloom_check(bloom_filter_t *B, key_t_ k){
    for (index_t i = 0; i < B->hashes; i++){
        index_t hash = (hash1(B, k) + i * hash3(B, k)) % B->size;

        if (!get_bit(B, hash)) return 0;
    }
    // possible case of a false positive
    return 1;
}
void bloom_add(bloom_filter_t *B, key_t_ k){
    for (index_t i = 0; i < B->hashes; i++){
        index_t hash = (hash1(B, k) + i * hash3(B, k)) % B->size;

        // Check if bit is already set
        if(!get_bit(B, hash)) {
            set_bit(B, hash);
            B->count++;
        }
    }
}

// int main(){

//     // Testing the hash

//     int test_keys[] = {0, 1, 2, 3, 13, 97};
//     for (int i=0; i<6; i++){
//         printf( "Key: %d , Hash1: %llu\n", test_keys[i], hash1(NULL, test_keys[i]));
//         printf( "Key: %d , Hash2: %llu\n", test_keys[i], hash2(NULL, test_keys[i]));
//     }

//     // Testing the Bloom filter

//     // Test 0: Basic functions
//     int n_hashes = 3;
//     bloom_filter_t *bloom = (bloom_filter_t*)malloc(sizeof(bloom_filter_t));
//     bloom_init(bloom, 1000, n_hashes);

//     // Adding the first 70 integers
//     for (key_t_ k=1UL; k < 71UL; k++){
//         bloom_add(bloom, k);
//     }    
//     key_t_ k1 = 1UL;
//     key_t_ k2 = 71UL;
//     printf("Test of check: \n key present  %d \n key absent %d \n",
//            bloom_check(bloom, k1), bloom_check(bloom, k2));
//     // clear object
//     bloom_destroy(bloom);

//     // Test 1: occupancy
//     n_hashes = 7;
//     index_t size_in_bits = 1000000;
//     key_t_ size = 100000UL; 
//     bloom = (bloom_filter_t*)malloc(sizeof(bloom_filter_t));
//     bloom_init(bloom, size_in_bits, n_hashes);

//     for (key_t_ k=1UL; k < size; k++){
//         bloom_add(bloom, k);
//     }

//     printf("number of bits set %llu / %llu\n", bloom->count, bloom->size);
//     // clear object
//     bloom_destroy(bloom);

//     // Test 2: evaluation on random numbers
//     bloom = (bloom_filter_t*)malloc(sizeof(bloom_filter_t));
//     bloom_init(bloom, size_in_bits, n_hashes);

//     index_t rand_max = 100000000;

//     // Fill the array
//     key_t_ top = 100000UL;
//     printf("RAND_MAX is %d\n", RAND_MAX);
//     int test_occurences = 0;
//     for(index_t i = 1UL; i <= top; i++){
//         key_t_ r = (key_t_)rand() % rand_max;
//         bloom_add(bloom, r);
//         if (bloom_check(bloom, r)) test_occurences++;
//     }
//     printf("Test occurences : %d / 100 \n", test_occurences);

//     // Counting presence of new random numbers
//     int occurences = 0;
//     for(index_t i = 1UL; i <= top; i++){
//         key_t_ r = (key_t_)rand() % rand_max;
//         if (bloom_check(bloom, r)) occurences++;
//     }

//     printf("occupancy: %d bits  false positives: %d\n", (int)bloom->count, occurences);

//     // Test: writing on disk 
//     FILE* fout = fopen("test_bloom", "wb");
//     printf("Size saved %llu\n", bloom->size);
//     fwrite(&bloom->size, sizeof(index_t), 1, fout);
//     printf("Count saved %llu\n", bloom->count);
//     fwrite(&bloom->count, sizeof(index_t), 1, fout);
//     fwrite(bloom->table, 1, (bloom->size) / 8, fout);
//     fclose(fout);

//     // Reading from disk
//     bloom_destroy(bloom);
//     bloom = (bloom_filter_t*)malloc(sizeof(bloom_filter_t));

//     FILE* fin = fopen("test_bloom", "rb");
//     index_t* size_new = (index_t *) malloc(sizeof(index_t));
//     fread(size_new, sizeof(index_t), 1, fin);
//     printf("Size read %llu\n", *size_new);
//     bloom_init(bloom, *size_new, HASHES);
//     fread(&bloom->count, sizeof(index_t), 1, fin);
//     printf("Count read %llu\n", bloom->count);
//     fread(bloom->table, 1, (bloom->size) / 8, fin);
//     fclose(fin);

// }