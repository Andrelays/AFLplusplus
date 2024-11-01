#ifndef PNG_MUTATOR_H_INCLUDED
#define PNG_MUTATOR_H_INCLUDED
// TODO: my lsp said, "Included header is not used directly"
//       avoid including header file in other header since it significantly increase compilation time
//
#include "afl-fuzz.h" // You need to use -I/path/to/AFLplusplus/include -I. 

static const char *const TYPE_ARRAY[] = {
"IDAT", "IEND", "IHDR", "PLTE", "bKGD",
"cHRM", "fRAc", "gAMA", "gIFg", "gIFt",
"gIFx", "hIST", "iCCP", "iTXt", "oFFs",
"pCAL", "pHYs", "sBIT", "sCAL", "sPLT",
"sRGB", "sTER", "tEXt", "tIME", "tRNS",
"zTXt", "fUZz" // special chunk for extra fuzzing hints
};

const size_t NUMBER_TYPES = 27;
const size_t TYPE_IHDR = 2;

const u8 PNG_SIG[]    = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
const u8 IEND_CHUNK[] = {0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

const size_t PNG_SIG_SIZE = 8;
const size_t MIN_BUF_SIZE = 33;

const u8     MAX_U8 = 255;

const size_t VALID_VALUE_IHDR_SIZE    = 0x0d;
const u8     VALID_VALUE_COMPRESSION  = 0;
const u8     VALID_VALUE_FILTER       = 0;
const size_t NUM_TYPE_INTERLACE_VALID = 2;

const u8       NUM_TYPE_COLOR        = 4;
const u8       NUM_TYPE_COMPRESSION  = 2;
const u8       NUM_TYPE_FILTER       = 2;
const u8       NUM_TYPE_INTERLACE    = 3;
const uint32_t MAX_WIDE              = 512;
const uint32_t MAX_HIGH              = 512;

const size_t OFFSET_LEN_IHDR         = 8;
const size_t OFFSET_TYPE_IHDR        = 12;
const size_t OFFSET_WIDE_IHDR        = 16;
const size_t OFFSET_HIGH_IHDR        = 20;
const size_t OFFSET_BIT_DEPTH_IHDR   = 24;
const size_t OFFSET_COLOR_TYPE_IHDR  = 25;
const size_t OFFSET_COMPRESSION_IHDR = 26;
const size_t OFFSET_FILTER_IHDR      = 27;
const size_t OFFSET_INTERLACE_IHDR   = 28;

const size_t OFFSET_TYPE_IN_CHUNK = 4;
const size_t OFFSET_DATA_IN_CHUNK = 8;

const size_t SIZE_TYPE             = 4;
const size_t SIZE_LEN              = 4;
const size_t SIZE_EMPTY_CHUNK      = 12;
const size_t MAX_SIZE_INSERT_CHUNK = 256;

#endif //PNG_MUTATOR_H_INCLUDED
