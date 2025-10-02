#ifndef BEJ_H_
#define BEJ_H_

/**
 * @file bej.h
 * @brief Public API and common BEJ definitions.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/* Forward decls of public structs */
typedef struct bej_br bej_br;
typedef struct bej_jsonw bej_jsonw;

typedef struct {
    uint8_t  fmt;       /**< bejTupleF (upper nibble conveys value format in tuples). */
    uint16_t seq;       /**< SequenceNumber within its cluster. */
    uint16_t child_off; /**< Absolute byte offset (from file start) to child cluster records. */
    uint16_t child_cnt; /**< Number of child entries in that cluster. */
    uint8_t  name_len;  /**< Length of the UTF-8 name including NUL terminator. */
    uint16_t name_off;  /**< Absolute byte offset (from file start) of the UTF-8 name. */
} bej_dict_entry;

typedef struct {
    bej_dict_entry* ent;   /**< Pointer to array of entries. */
    size_t          n;     /**< Number of entries. */
    size_t          entries_ofs; /**< Absolute file offset where entries array begins. */
    size_t          names_ofs;   /**< Absolute file offset where the names pool begins. */
    const uint8_t*  blob;  /**< Raw dictionary blob (for name access). */
    size_t          blob_n;/**< Size of blob in bytes. */
} bej_dict;

typedef struct {
    uint32_t start_idx;
    uint16_t count;
} bej_cluster;

/** @name BEJ format nibble values (upper 4 bits of bejTupleF) @{ */
#define BEJ_FMT_SET     0x0
#define BEJ_FMT_ARRAY   0x1
#define BEJ_FMT_NULL    0x2
#define BEJ_FMT_INT     0x3
#define BEJ_FMT_ENUM    0x4
#define BEJ_FMT_STRING  0x5
/** @} */

/* Reader API */
struct bej_br {
    const uint8_t* d;
    size_t n;
    size_t p;
};
void     bej_br_init(bej_br* b, const uint8_t* d, size_t n);
int      bej_br_u8(bej_br* b, uint8_t* v);
int      bej_br_get(bej_br* b, uint8_t* dst, size_t k);
int      bej_br_seek(bej_br* b, size_t pos);
int      bej_br_left(const bej_br* b);
int      bej_read_nnint(bej_br* b, uint64_t* out);

/* JSON writer API */
struct bej_jsonw {
    FILE* f;
    int   ind;
    int   need_comma;
};
void bej_jw_init(bej_jsonw* j, FILE* f);
void bej_jw_nl(bej_jsonw* j);
void bej_jw_begin_obj(bej_jsonw* j);
void bej_jw_end_obj(bej_jsonw* j);
void bej_jw_begin_arr(bej_jsonw* j);
void bej_jw_end_arr(bej_jsonw* j);
void bej_jw_key(bej_jsonw* j, const char* k);
void bej_jw_str(bej_jsonw* j, const char* s);
void bej_jw_int(bej_jsonw* j, long long v);

/* Dictionary API */
int  bej_dict_load(const uint8_t* d, size_t n, bej_dict* out);
void bej_dict_free(bej_dict* D);
const char* bej_dict_name_at(const bej_dict* D, uint16_t name_off);
const bej_dict_entry* bej_cluster_lookup_seq(const bej_dict* D, bej_cluster c, uint16_t seq);

/* Decoder API */
int  bej_decode_to_json(FILE* out, const uint8_t* bej, size_t bej_n, const bej_dict* D);

#endif /* BEJ_H_ */
#ifndef BEJ_H_
#define BEJ_H_

/**
 * @file bej.h
 * @brief Public API and common BEJ definitions.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/* Forward decls of public structs */
typedef struct bej_br bej_br;
typedef struct bej_jsonw bej_jsonw;

typedef struct {
    uint8_t  fmt;       /**< bejTupleF (upper nibble conveys value format in tuples). */
    uint16_t seq;       /**< SequenceNumber within its cluster. */
    uint16_t child_off; /**< Absolute byte offset (from file start) to child cluster records. */
    uint16_t child_cnt; /**< Number of child entries in that cluster. */
    uint8_t  name_len;  /**< Length of the UTF-8 name including NUL terminator. */
    uint16_t name_off;  /**< Absolute byte offset (from file start) of the UTF-8 name. */
} bej_dict_entry;

typedef struct {
    bej_dict_entry* ent;   /**< Pointer to array of entries. */
    size_t          n;     /**< Number of entries. */
    size_t          entries_ofs; /**< Absolute file offset where entries array begins. */
    size_t          names_ofs;   /**< Absolute file offset where the names pool begins. */
    const uint8_t*  blob;  /**< Raw dictionary blob (for name access). */
    size_t          blob_n;/**< Size of blob in bytes. */
} bej_dict;

typedef struct {
    uint32_t start_idx;
    uint16_t count;
} bej_cluster;

/** @name BEJ format nibble values (upper 4 bits of bejTupleF) @{ */
#define BEJ_FMT_SET     0x0
#define BEJ_FMT_ARRAY   0x1
#define BEJ_FMT_NULL    0x2
#define BEJ_FMT_INT     0x3
#define BEJ_FMT_ENUM    0x4
#define BEJ_FMT_STRING  0x5
/** @} */

/* Reader API */
struct bej_br {
    const uint8_t* d;
    size_t n;
    size_t p;
};
void     bej_br_init(bej_br* b, const uint8_t* d, size_t n);
int      bej_br_u8(bej_br* b, uint8_t* v);
int      bej_br_get(bej_br* b, uint8_t* dst, size_t k);
int      bej_br_seek(bej_br* b, size_t pos);
int      bej_br_left(const bej_br* b);
int      bej_read_nnint(bej_br* b, uint64_t* out);

/* JSON writer API */
struct bej_jsonw {
    FILE* f;
    int   ind;
    int   need_comma;
};
void bej_jw_init(bej_jsonw* j, FILE* f);
void bej_jw_nl(bej_jsonw* j);
void bej_jw_begin_obj(bej_jsonw* j);
void bej_jw_end_obj(bej_jsonw* j);
void bej_jw_begin_arr(bej_jsonw* j);
void bej_jw_end_arr(bej_jsonw* j);
void bej_jw_key(bej_jsonw* j, const char* k);
void bej_jw_str(bej_jsonw* j, const char* s);
void bej_jw_int(bej_jsonw* j, long long v);

/* Dictionary API */
int  bej_dict_load(const uint8_t* d, size_t n, bej_dict* out);
void bej_dict_free(bej_dict* D);
const char* bej_dict_name_at(const bej_dict* D, uint16_t name_off);
const bej_dict_entry* bej_cluster_lookup_seq(const bej_dict* D, bej_cluster c, uint16_t seq);

/* Decoder API */
int  bej_decode_to_json(FILE* out, const uint8_t* bej, size_t bej_n, const bej_dict* D);

#endif /* BEJ_H_ */
