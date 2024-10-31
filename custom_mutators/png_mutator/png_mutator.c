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

const size_t MAX_NUM_GENERATE_CHUNK = 5;
const size_t MIN_NUM_GENERATE_CHUNK = 3;

static void   fill_random  (u8 *arr, size_t len, u8 max);
static size_t insert_chunk (u8 *insert_buf);
static size_t insert_ihdr  (u8 *insert_buf);

typedef struct my_mutator {
    afl_state_t *afl;

    u8 *mutated_out;
    u8 *post_process_buf;

} my_mutator_t;

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

    ALLOCATE_MEMORY_(data->mutated_out,      data);
    ALLOCATE_MEMORY_(data->post_process_buf, data->mutated_out, data);

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
    size_t max_num_insert_chunk = (size_t)rand() % (MAX_NUM_GENERATE_CHUNK - MIN_NUM_GENERATE_CHUNK) + MIN_NUM_GENERATE_CHUNK;
    size_t mutated_size = 0;

    memcpy(data->mutated_out, PNG_SIG, PNG_SIG_SIZE);
    mutated_size += PNG_SIG_SIZE;

    mutated_size += insert_ihdr(data->mutated_out);

    for(size_t num_insert_chunk = 0; num_insert_chunk <= max_num_insert_chunk; num_insert_chunk++) {
        mutated_size += insert_chunk(data->mutated_out + mutated_size);
    }

    memcpy(data->mutated_out, IEND_CHUNK, SIZE_EMPTY_CHUNK);
    data->mutated_out += SIZE_EMPTY_CHUNK;

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

static size_t insert_ihdr(u8 *insert_buf)
{
    *(uint32_t *)(insert_buf + OFFSET_LEN_IHDR) = VALID_VALUE_IHDR_SIZE;

    memcpy(insert_buf + OFFSET_TYPE_IHDR, TYPE_ARRAY[TYPE_IHDR], SIZE_TYPE);

    *(uint32_t *)(insert_buf + OFFSET_WIDE_IHDR) = (u8)rand() % MAX_WIDE;
    *(uint32_t *)(insert_buf + OFFSET_HIGH_IHDR) = (u8)rand() % MAX_HIGH;

    insert_buf[OFFSET_BIT_DEPTH_IHDR]   = (u8)rand() % MAX_U8;
    insert_buf[OFFSET_COLOR_TYPE_IHDR]  = (u8)rand() % NUM_TYPE_COLOR;
    insert_buf[OFFSET_COMPRESSION_IHDR] = VALID_VALUE_COMPRESSION;
    insert_buf[OFFSET_FILTER_IHDR]      = VALID_VALUE_FILTER;
    insert_buf[OFFSET_INTERLACE_IHDR]   = (u8)rand() % NUM_TYPE_INTERLACE_VALID;

    return VALID_VALUE_IHDR_SIZE + SIZE_EMPTY_CHUNK;
}

static void fill_random(u8 *arr, size_t len, u8 max)
{
    for (size_t index_arr = 0; index_arr < len; index_arr++) {
        arr[index_arr] = (u8) rand() % max;
    }

    return;
}


/**
 * A post-processing function to use right before AFL writes the test case to
 * disk in order to execute the target.
 *
 * @param[in]  data     Pointer returned in afl_custom_init for this fuzz case
 * @param[in]  in_buf   Buffer containing the test case to be executed
 * @param[in]  buf_size Size of the test case
 * @param[out] out_buf  Pointer to the buffer containing the test case after
 *     processing. External library should allocate memory for out_buf.
 *     The buf pointer may be reused (up to the given buf_size);
 * @return Size of the output buffer after processing or the needed amount.
 *     A return of 0 indicates an error.
 */
size_t afl_custom_post_process(my_mutator_t *data, const u8 *in_buf, size_t buf_size, const u8 **out_buf)
{
    memcpy(data->post_process_buf, in_buf, buf_size);

    if(buf_size < MIN_BUF_SIZE)
    {
      *out_buf = data->post_process_buf;
      return buf_size;
    }

    size_t len_chunk_data = 0;

    for(size_t pos_in_buf = PNG_SIG_SIZE; pos_in_buf + SIZE_EMPTY_CHUNK <= buf_size; pos_in_buf += SIZE_EMPTY_CHUNK + len_chunk_data)
    {
        u8* chunk = data->post_process_buf + pos_in_buf;

        len_chunk_data = htonl(*(uint32_t *)chunk);

        if(len_chunk_data > MAX_FILE || pos_in_buf + 12 + len_chunk_data > buf_size || pos_in_buf + len_chunk_data + 12 < pos_in_buf) {
            break;
        }

        uint32_t real_cksum = (uint32_t) crc32(0, chunk + OFFSET_TYPE_IN_CHUNK, (unsigned)(len_chunk_data + SIZE_TYPE));
        uint32_t file_cksum = htonl(*(uint32_t *)(chunk + OFFSET_DATA_IN_CHUNK + len_chunk_data));

        if(file_cksum != real_cksum) {
            *(uint32_t *)(chunk + OFFSET_DATA_IN_CHUNK + len_chunk_data) = real_cksum;
        }
    }

    *out_buf = data->post_process_buf;
    return buf_size;
}

void afl_custom_deinit(my_mutator_t *data)
{
    free(data->mutated_out);
    free(data->post_process_buf);

    free(data);

    return;
}
