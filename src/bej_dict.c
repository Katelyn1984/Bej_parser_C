/**
 * @file bej_dict.c
 * @brief Redfish schema dictionary (DSP0218 Table 31) parser.
 */

#include <stdlib.h>
#include "bej.h"

/**
 * @brief Parse a Redfish schema dictionary binary (Table 31).
 *
 * @param d Pointer to dictionary blob.
 * @param n Size of dictionary blob.
 * @param out Output parsed dictionary.
 * @return 1 on success, 0 on failure (format mismatch or truncation).
 */
int bej_dict_load(const uint8_t* d, size_t n, bej_dict* out){
    if(!d || !out) return 0;
    if(n < 12) return 0;
    size_t p=0;
    uint8_t verTag = d[p++];           /* ignored here */
    uint8_t flags  = d[p++];           /* ignored here */
    uint16_t entryCount = (uint16_t)(d[p] | (d[p+1]<<8)); p+=2;
    (void)flags; (void)verTag;
    uint32_t schemaVer = (uint32_t)(d[p] | (d[p+1]<<8) | (d[p+2]<<16) | (d[p+3]<<24)); p+=4;
    uint32_t dictSize  = (uint32_t)(d[p] | (d[p+1]<<8) | (d[p+2]<<16) | (d[p+3]<<24)); p+=4;
    (void)schemaVer; (void)dictSize;
    if(n < p + entryCount*10) return 0;

    bej_dict_entry* a = (bej_dict_entry*)calloc(entryCount, sizeof(bej_dict_entry));
    if(!a) return 0;
    size_t entries_ofs = p;

    for(uint16_t i=0;i<entryCount;i++){
        a[i].fmt       = d[p+0];
        a[i].seq       = (uint16_t)(d[p+1] | (d[p+2]<<8));
        a[i].child_off = (uint16_t)(d[p+3] | (d[p+4]<<8));
        a[i].child_cnt = (uint16_t)(d[p+5] | (d[p+6]<<8));
        a[i].name_len  = d[p+7];
        a[i].name_off  = (uint16_t)(d[p+8] | (d[p+9]<<8));
        p += 10;
    }

    size_t names_ofs = p;
    out->ent=a; out->n=entryCount; out->entries_ofs=entries_ofs; out->names_ofs=names_ofs; out->blob=d; out->blob_n=n;
    return 1;
}

/** Free dictionary (entries are heap-allocated; name strings point into blob). */
void bej_dict_free(bej_dict* D){
    if(!D) return;
    free(D->ent); D->ent=NULL; D->n=0; D->entries_ofs=D->names_ofs=0; D->blob=NULL; D->blob_n=0;
}

/**
 * @brief Get a pointer to a null-terminated field name at a given name offset.
 *
 * @param D Parsed dictionary.
 * @param name_off Absolute file offset of the string.
 * @return Pointer to UTF-8 name within @ref bej_dict::blob, or NULL if invalid.
 */
const char* bej_dict_name_at(const bej_dict* D, uint16_t name_off){
    if(!D) return NULL;
    if(name_off==0) return NULL;
    if(name_off >= D->blob_n) return NULL;
    const char* s = (const char*)(D->blob + name_off);
    /* ensure NUL-terminated within bounds */
    size_t i=name_off;
    while(i < D->blob_n && D->blob[i]!=0) i++;
    if(i>=D->blob_n) return NULL;
    return s;
}

/**
 * @brief Lookup an entry within a cluster by logical sequence number.
 * @param D Dictionary.
 * @param c Cluster (start index and number of entries).
 * @param seq Sequence number to search for.
 * @return Pointer to the matching entry within D, or NULL if not found.
 */
const bej_dict_entry* bej_cluster_lookup_seq(const bej_dict* D, bej_cluster c, uint16_t seq){
    if(!D) return NULL;
    uint32_t i = c.start_idx;
    uint32_t end = i + c.count;
    if(end > D->n) end = (uint32_t)D->n;
    for(; i<end; ++i){
        if(D->ent[i].seq == seq) return &D->ent[i];
    }
    return NULL;
}
