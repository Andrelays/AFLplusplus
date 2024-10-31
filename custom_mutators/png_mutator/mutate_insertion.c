/*
   Compile with:

     gcc -shared -Wall -O3 post_library_png.so.c -o post_library_png.so -lz

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>
#include <arpa/inet.h>
#include "png_mutator.h"

#define IS_RAND_33_PERCENT_PROBABILITY !(rand() % 3)

typedef struct my_mutator {
    afl_state_t *afl;

    u8 *mutated_out;

} my_mutator_t;

static void   fill_random  (u8 *arr, size_t len, u8 max);
static size_t insert_chunk (u8 *insert_buf);

/**
 * Initialize this custom mutator
 *
 * @param[in] afl a pointer to the internal state object. Can be ignored for
 * now.
 * @param[in] seed A seed for this mutator - the same seed should always mutate
 * in the same way.
 * @return Pointer to the data object this custom mutator instance should use.
 *         There may be multiple instances of this mutator in one afl-fuzz run!
 *         Return NULL on error.
 */
my_mutator_t *afl_custom_init(afl_state_t *afl, uint32_t seed)
{
    #define ALLOCATE_MEMORY_(buf, ...)                                  \
    do {                                                                \
        buf = (u8 *)calloc(MAX_FILE, sizeof(u8));                       \
                                                                        \
        if (!buf)                                                       \
        {                                                               \
            void *pta[] = {__VA_ARGS__};                                \
            for(size_t i = 0; i < sizeof(pta)/sizeof(void*); i++) {     \
                free(pta[i]);                                           \
            }                                                           \
                                                                        \
            perror("Afl_custom_init alloc_##buf");                      \
            return NULL;                                                \
        }                                                               \
                                                                        \
    } while(0)

    srand(seed); // needed also by surgical_havoc_mutate()

    my_mutator_t *data = (my_mutator_t *)calloc(1, sizeof(my_mutator_t));
    if (!data)
    {
        perror("Afl_custom_init alloc data\n");
        return NULL;
    }

    ALLOCATE_MEMORY_(data->mutated_out, data);

    data->afl = afl;

    return data;

    #undef ALLOCATE_MEMORY_
}

/**
 * Perform custom mutations on a given input
 *
 * (Optional for now. Required in the future)
 *
 * @param[in] data     Pointer returned in afl_custom_init for this fuzz case
 * @param[in] in_buf   Pointer to input data to be mutated
 * @param[in] buf_size Size of input data
 * @param[out] out_buf the buffer we will work on. we can reuse *buf. NULL on
 * error.
 * @param[in] add_buf      Buffer containing the additional test case
 * @param[in] add_buf_size Size of the additional test case
 * @param[in] max_size     Maximum size of the mutated output. The mutation must not
 * produce data larger than max_size.
 * @return                 Size of the mutated output.
 */
size_t afl_custom_fuzz(my_mutator_t *data, const u8 *in_buf, size_t buf_size, u8 **out_buf, u8 *add_buf, size_t add_buf_size, size_t max_size)
{
    size_t mutated_size = buf_size <= PNG_SIG_SIZE ? buf_size : PNG_SIG_SIZE;

    memcpy(data->mutated_out, in_buf, mutated_size);

    if (mutated_size < PNG_SIG_SIZE) {
        *out_buf = data->mutated_out;
        return mutated_size;
    }

    size_t len_chunk_data = 0;

    for(size_t pos_in_buf = PNG_SIG_SIZE; pos_in_buf + SIZE_EMPTY_CHUNK <= buf_size; pos_in_buf += SIZE_EMPTY_CHUNK + len_chunk_data)
    {
        if(IS_RAND_33_PERCENT_PROBABILITY) {
            mutated_size += insert_chunk(data->mutated_out + mutated_size);
        }

        len_chunk_data = htonl(*(const uint32_t *)(in_buf + pos_in_buf));

        if(len_chunk_data > MAX_FILE || pos_in_buf + SIZE_EMPTY_CHUNK + len_chunk_data > buf_size || pos_in_buf + len_chunk_data + 12 < pos_in_buf) {
            break;
        }

        memcpy(data->mutated_out + mutated_size, in_buf + pos_in_buf, len_chunk_data + SIZE_EMPTY_CHUNK);

        mutated_size += len_chunk_data + SIZE_EMPTY_CHUNK;
    }

    mutated_size = mutated_size <= max_size ? mutated_size : max_size;

    *out_buf = data->mutated_out;

    return mutated_size;
}

static size_t insert_chunk(u8 *insert_buf)
{
    size_t len_chunk_data     = (size_t)rand() % MAX_SIZE_INSERT_CHUNK;
    *(uint32_t *)(insert_buf) = (uint32_t)len_chunk_data;

    memcpy(insert_buf + OFFSET_TYPE_IN_CHUNK, TYPE_ARRAY[(size_t)rand() % (NUMBER_TYPES)], SIZE_TYPE);

    fill_random(insert_buf + OFFSET_DATA_IN_CHUNK, len_chunk_data, MAX_U8);

    uint32_t cksum = (uint32_t) crc32(0, insert_buf + OFFSET_TYPE_IN_CHUNK, (unsigned)(len_chunk_data + SIZE_TYPE));

   *(uint32_t *)(insert_buf + OFFSET_DATA_IN_CHUNK + len_chunk_data) = cksum;

    return len_chunk_data + SIZE_EMPTY_CHUNK;
}

static void fill_random(u8 *arr, size_t len, u8 max)
{
    for (size_t index_arr = 0; index_arr < len; index_arr++) {
        arr[index_arr] = (u8)rand() % max;
    }

    return;
}

/**
 * Deinitialize everything
 *
 * @param data The data ptr from afl_custom_init
 */
void afl_custom_deinit(my_mutator_t *data)
{
    free(data->mutated_out);
    free(data);

    return;
}

