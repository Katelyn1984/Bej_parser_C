/**
 * @file bej_reader.c
 * @brief Byte reader and nnint utilities.
 */

#include <string.h>
#include "bej.h"

/**
 * @brief Initialize a byte reader over a given buffer.
 * @param b Reader instance to initialize.
 * @param d Pointer to the buffer data.
 * @param n Size of the buffer in bytes.
 */
void bej_br_init(bej_br* b, const uint8_t* d, size_t n){ b->d=d; b->n=n; b->p=0; }

/**
 * @brief Read one byte from the reader.
 * @param b Reader.
 * @param v Output: byte read.
 * @return 1 on success, 0 if out of bounds.
 */
int  bej_br_u8 (bej_br* b, uint8_t* v){ if(b->p>=b->n) return 0; *v=b->d[b->p++]; return 1; }

/**
 * @brief Read a raw block of bytes.
 * @param b Reader.
 * @param dst Destination buffer to copy into.
 * @param k Number of bytes to copy.
 * @return 1 on success, 0 on overflow.
 */
int  bej_br_get(bej_br* b, uint8_t* dst, size_t k){ if(b->p+k>b->n) return 0; memcpy(dst, b->d+b->p, k); b->p+=k; return 1; }

/**
 * @brief Seek to an absolute position within the buffer.
 * @param b Reader.
 * @param pos Absolute position.
 * @return 1 on success, 0 if out of range.
 */
int  bej_br_seek(bej_br* b, size_t pos){ if(pos>b->n) return 0; b->p=pos; return 1; }

/**
 * @brief Get number of bytes remaining (n - p).
 * @param b Reader.
 * @return Remaining byte count (may be zero).
 */
int  bej_br_left(const bej_br* b){ return (int)(b->n - b->p); }

/**
 * @brief Read a BEJ non-negative integer (nnint) as per DSP0218.
 *
 * Encoding is: a single length byte N, followed by N bytes containing
 * a little-endian unsigned integer value.
 *
 * @param b Reader positioned at the start of an nnint.
 * @param out Output decoded value.
 * @return 1 on success, 0 on malformed/overflow.
 */
int bej_read_nnint(bej_br* b, uint64_t* out){
    uint8_t N; if(!bej_br_u8(b,&N)) return 0;
    uint64_t v=0;
    if(b->p+N > b->n) return 0;
    for(unsigned i=0;i<N;i++) v |= (uint64_t)b->d[b->p+i] << (8*i);
    b->p += N;
    *out = v;
    return 1;
}
