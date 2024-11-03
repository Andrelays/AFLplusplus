/*
   Compile with:

     gcc -shared -Wall -O3 mutate_png.c -o mutate_png.so -lz

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <zlib.h> //for crc32
#include "afl-fuzz.h"

const size_t PNG_SIG_SIZE = 8;

const size_t SIZE_EMPTY_CHUNK      = 12;
const size_t MAX_SIZE_INSERT_CHUNK = 256;

const size_t OFFSET_TYPE_IN_CHUNK = 4;
const size_t OFFSET_DATA_IN_CHUNK = 8;

const size_t SIZE_TYPE = 4;

static const char *const TYPE_ARRAY[] = {
"IDAT", "IEND", "IHDR", "PLTE", "bKGD",
"cHRM", "fRAc", "gAMA", "gIFg", "gIFt",
"gIFx", "hIST", "iCCP", "iTXt", "oFFs",
"pCAL", "pHYs", "sBIT", "sCAL", "sPLT",
"sRGB", "sTER", "tEXt", "tIME", "tRNS",
"zTXt", "fUZz" // special chunk for extra fuzzing hints
};

const size_t NUMBER_TYPES = 27;


typedef struct my_mutator {
    afl_state_t *afl;

    u8 *mutated_out;

} my_mutator_t;

static void   fill_random  (u8 *arr, size_t len, u8 max);
static size_t insert_chunk (u8 *insert_buf);
static bool   is_rand_33_percent_probability();

my_mutator_t *afl_custom_init(afl_state_t *afl, uint32_t seed)
{
    srand(seed); // needed also by surgical_havoc_mutate()

    my_mutator_t *data = (my_mutator_t *)calloc(1, sizeof(my_mutator_t));
    if (!data)
    {
        perror("Afl_custom_init alloc data\n");
        return NULL;
    }

    data->mutated_out = (u8 *)calloc(MAX_FILE, sizeof(u8));
    if(!data->mutated_out)
    {
        free(data);
        perror("Afl_custom_init alloc data\n");
        return NULL;
    }

    data->afl = afl;

    return data;
}

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
        if(is_rand_33_percent_probability()) {
            mutated_size += insert_chunk(data->mutated_out + mutated_size);
        }

        len_chunk_data = htonl(*(const uint32_t *)(in_buf + pos_in_buf));

        if(len_chunk_data > MAX_FILE || pos_in_buf + SIZE_EMPTY_CHUNK + len_chunk_data > buf_size || pos_in_buf + len_chunk_data + 12 < pos_in_buf) {
            break;
        }

        if(is_rand_33_percent_probability()) {
            continue;
        }

        memcpy(data->mutated_out + mutated_size, in_buf + pos_in_buf, len_chunk_data + SIZE_EMPTY_CHUNK);

        if(is_rand_33_percent_probability()) {
            fill_random(data->mutated_out + mutated_size + OFFSET_DATA_IN_CHUNK, len_chunk_data, UCHAR_MAX);
        }

        if(is_rand_33_percent_probability()) {
            memcpy(data->mutated_out + mutated_size + OFFSET_TYPE_IN_CHUNK, TYPE_ARRAY[(size_t)rand() % (NUMBER_TYPES)], SIZE_TYPE);
        }

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

    fill_random(insert_buf + OFFSET_DATA_IN_CHUNK, len_chunk_data, UCHAR_MAX);

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

static bool is_rand_33_percent_probability()
{
    return (rand() % 3);
}

void afl_custom_deinit(my_mutator_t *data)
{
    free(data->mutated_out);
    free(data);

    return;
}

