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

const size_t NUM_MUTATION_OPTION = 9;

typedef enum mutation_ihdr {
    MUTATE_LEN          = 0,
    MUTATE_TYPE         = 1,
    MUTATE_WIDE         = 2,
    MUTATE_HIGH         = 3,
    MUTATE_BIT_DEPTH    = 4,
    MUTATE_COLOR_TYPE   = 5,
    MUTATE_COMPRESSION  = 6,
    MUTATE_FILTER       = 7,
    MUTATE_INTERLACE    = 8

} mutation_ihdr_t;

typedef struct my_mutator {
    afl_state_t *afl;

    u8 *mutated_out;

} my_mutator_t;

static void fill_random(u8 *arr, size_t len, u8 max);

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
    size_t mutated_size = buf_size <= max_size ? buf_size : max_size;

    memcpy(data->mutated_out, in_buf, mutated_size);

    if (mutated_size < MIN_BUF_SIZE) {
        *out_buf = data->mutated_out;
        return mutated_size;
    }

    switch ((mutation_ihdr_t)((size_t)rand() % NUM_MUTATION_OPTION))
    {
        case MUTATE_LEN:
            fill_random(data->mutated_out + OFFSET_LEN_IHDR, SIZE_LEN, MAX_U8);
            break;

        case MUTATE_TYPE:
            fill_random(data->mutated_out + OFFSET_TYPE_IHDR, SIZE_TYPE, MAX_U8);
            break;

        case MUTATE_WIDE:
            *(uint32_t *)(data->mutated_out + OFFSET_WIDE_IHDR) = (u8)rand() % MAX_WIDE;
            break;

        case MUTATE_HIGH:
            *(uint32_t *)(data->mutated_out + OFFSET_HIGH_IHDR) = (u8)rand() % MAX_HIGH;
            break;

        case MUTATE_BIT_DEPTH:
            data->mutated_out[OFFSET_BIT_DEPTH_IHDR] = (u8)rand() % MAX_U8;
            break;

        case MUTATE_COLOR_TYPE:
            data->mutated_out[OFFSET_COLOR_TYPE_IHDR] = (u8)rand() % NUM_TYPE_COLOR;
            break;

        case MUTATE_COMPRESSION:
            data->mutated_out[OFFSET_COMPRESSION_IHDR] = (u8)rand() % NUM_TYPE_COMPRESSION;
            break;

        case MUTATE_FILTER:
            data->mutated_out[OFFSET_FILTER_IHDR] = (u8)rand() % NUM_TYPE_FILTER;
            break;

        case MUTATE_INTERLACE:
            data->mutated_out[OFFSET_INTERLACE_IHDR] = (u8)rand() % NUM_TYPE_INTERLACE;
            break;

        default:
            perror("Unknown mutation\n");
            break;
    }


    *out_buf = data->mutated_out;

    return mutated_size;
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

static void fill_random(u8 *arr, size_t len, u8 max)
{
    for (size_t index_arr = 0; index_arr < len; index_arr++) {
        arr[index_arr] = (u8)rand() % max;
    }

    return;
}
