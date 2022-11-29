/*
 * zone.h -- zone parser.
 *
 * Copyright (c) 2022, NLnet Labs. All rights reserved.
 *
 * See LICENSE for the license.
 *
 */
#ifndef ZONE_H
#define ZONE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef int32_t zone_return_t;

typedef struct zone_string zone_string_t;
struct zone_string {
  const uint8_t *data;
  size_t length;
};

// @private
typedef struct zone_token zone_token_t;
struct zone_token {
  zone_string_t string;
  size_t line;
};

typedef struct zone_name zone_name_t;
struct zone_name {
  size_t length;
  uint8_t octets[256];
};

typedef int32_t zone_code_t;

// types are defined by their binary representation. this is different from
// dnsextlang, which defines types mostly by their textual representation.
// e.g. I4, T and T[L] are different field types in dnsextlang, but the wire
// format is identical. qualifiers, like time and ttl, are available from the
// field descriptor for completeness.
typedef enum {
  ZONE_INT8 = (1 << 14),
  ZONE_INT16 = (2 << 14),
  ZONE_INT32 = (3 << 14),
  ZONE_IP4 = (4 << 14),
  ZONE_IP6 = (5 << 14),
  ZONE_NAME = (6 << 14),
  ZONE_STRING = (1 << 8), // (used by scanner)
  // (B)inary (L)arge (Ob)ject. Inspired by relation database terminology.
  // Must be last.
  ZONE_BLOB = (7 << 14),
  // hex fields
  // ZONE_EUI48 (ZONE_HEX6?)
  // ZONE_EUI64 (ZONE_HEX8?)
  // miscellaneous fields
  ZONE_SVC_PARAM = (1 << 9), // (used by scanner)
  ZONE_WKS = (8 << 14),
  ZONE_NSEC = (9 << 14)
} zone_type_t;

typedef enum {
  ZONE_TTL = (1 << 0), // may be used as qualifier for int32 rdata
  ZONE_CLASS = (1 << 1),
  ZONE_TYPE = (1 << 2), // may be used as qualifier for int16 rdata
  ZONE_DELIMITER = (1 << 3),
  ZONE_OWNER = (2 << 3),
  ZONE_RDATA = (3 << 3)
} zone_item_t;

#define ZONE_BLOCK_SIZE (64)

// tape capacity must be large enough to hold every token from a single
// worst-case read (e.g. 64 consecutive line feeds). in practice a single
// block will never contain 64 tokens, therefore, to optimize throughput,
// allocate twice the size so consecutive index operations can be done
#define ZONE_TAPE_SIZE (100 * (ZONE_BLOCK_SIZE + ZONE_BLOCK_SIZE))

// @private
typedef struct zone_file zone_file_t;
struct zone_file {
  zone_file_t *includer;
  struct {
    const void *domain;
    zone_name_t name;
  } origin, owner;
  uint32_t ttl;
  size_t line;
  const char *name;
  const char *path;
  int handle;
  int end_of_file;
  struct {
    size_t index, length, size;
    char *data;
  } buffer;
  // indexer state is stored per-file
  struct {
    uint64_t in_comment;
    uint64_t in_quoted;
    uint64_t is_escaped;
    uint64_t follows_contiguous;
    // vector of tokens generated by the indexer. guaranteed to be large
    // enough to hold every token for a single read + terminator
    //size_t index;
    size_t *head, *tail, tape[ZONE_TAPE_SIZE + 1];
  } indexer;
};

typedef void *(*zone_malloc_t)(void *arena, size_t size);
typedef void *(*zone_realloc_t)(void *arena, void *ptr, size_t size);
typedef void(*zone_free_t)(void *arena, void *ptr);

typedef struct zone_options zone_options_t;
struct zone_options {
  // FIXME: add a flags member. e.g. to allow for includes in combination
  //        with static buffers, signal ownership of allocated memory, etc
  // FIXME: a compiler flag indicating host or network order might be useful
  uint32_t flags;
  const char *origin;
  uint32_t ttl;
  size_t block_size; // >> as in a multiple of 64 for performance and to serve
                     //    the indexer
  struct {
    zone_malloc_t malloc;
    zone_realloc_t realloc;
    zone_free_t free;
    void *arena;
  } allocator;
//  struct {
//    zone_accept_name_t name;
//    zone_accept_rr_t rr;
//    zone_accept_rdata_t rdata;
//    zone_accept_t delimiter;
//  } accept;
};

typedef struct zone_parser zone_parser_t;
struct zone_parser {
  zone_options_t options;
  zone_file_t first, *file;
  struct {
    zone_code_t scanner;
  } state;
};

// return codes
#define ZONE_SUCCESS (0)
#define ZONE_SYNTAX_ERROR (-1)
#define ZONE_SEMANTIC_ERROR (-2)
#define ZONE_OUT_OF_MEMORY (-3)
#define ZONE_BAD_PARAMETER (-4)
#define ZONE_READ_ERROR (-5)
#define ZONE_NOT_IMPLEMENTED (-6)

zone_return_t zone_open(
  zone_parser_t *parser, const zone_options_t *options, const char *file);

void zone_close(
  zone_parser_t *parser);

// FIXME: zone_process will merely be a proxy function to call the actual
//        architecture specific function (SSE4, AVX2, NEON, etc) implemented
//        in zone.c
zone_return_t zone_process(
  zone_parser_t *parser, void *user_data);

#endif // ZONE_H
