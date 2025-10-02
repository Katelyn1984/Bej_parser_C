/* tests/test_bej_c.c
 * Minimal C unit tests for BEJ, no external deps, GCC 6.x friendly.
 * Covers: nnint decoding (two cases) and dictionary load + cluster lookup.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../src/bej.h"

/* --------------------- tiny test "framework" --------------------- */

static int g_failures = 0;
#define MU_ASSERT(cond) do { \
    if(!(cond)) { \
        fprintf(stderr, "[FAIL] %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        g_failures++; \
        return; \
    } \
} while(0)

#define MU_CHECK(cond) do { \
    if(!(cond)) { \
        fprintf(stderr, "[FAIL] %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        g_failures++; \
    } \
} while(0)

#define TEST(name) static void name(void)
#define RUN_TEST(fn) do { \
    fprintf(stdout, "[RUN ] %s\n", #fn); \
    fn(); \
    if(g_failures==before) fprintf(stdout, "[ OK ] %s\n", #fn); \
    else fprintf(stdout, "[ NG ] %s\n", #fn); \
} while(0)

/* --------------------- small helpers --------------------- */

static void push_u8(uint8_t** p, size_t* n, uint8_t v){
    (*p)[(*n)++] = v;
}
static void push_u16le(uint8_t** p, size_t* n, uint16_t x){
    (*p)[(*n)++] = (uint8_t)(x & 0xFF);
    (*p)[(*n)++] = (uint8_t)((x>>8) & 0xFF);
}
static void push_u32le(uint8_t** p, size_t* n, uint32_t x){
    (*p)[(*n)++] = (uint8_t)(x & 0xFF);
    (*p)[(*n)++] = (uint8_t)((x>>8) & 0xFF);
    (*p)[(*n)++] = (uint8_t)((x>>16)& 0xFF);
    (*p)[(*n)++] = (uint8_t)((x>>24)& 0xFF);
}
static void push_nnint(uint8_t** p, size_t* n, uint64_t val){
    uint8_t buf[8]; unsigned N=0;
    if(val==0){ N=1; buf[0]=0; }
    else { while(val){ buf[N++] = (uint8_t)(val & 0xFF); val >>= 8; } }
    push_u8(p,n,(uint8_t)N);
    for(unsigned i=0;i<N;i++) push_u8(p,n,buf[i]);
}
static void push_cstr(uint8_t** p, size_t* n, const char* s){
    while(*s) push_u8(p,n,(uint8_t)*s++);
    push_u8(p,n,0);
}

/* --------------------- tests --------------------- */

/* 1) nnint: 0 и 300 */
TEST(test_nnint_basic){
    /* 0 -> 01 00 */
    {
        uint8_t buf[2] = {0x01,0x00};
        bej_br br; bej_br_init(&br, buf, 2);
        uint64_t v=123;
        MU_ASSERT(bej_read_nnint(&br,&v)==1);
        MU_ASSERT(v==0);
        MU_ASSERT(bej_br_left(&br)==0);
    }
    /* 300 -> 02 2C 01 */
    {
        uint8_t buf[3] = {0x02,0x2C,0x01};
        bej_br br; bej_br_init(&br, buf, 3);
        uint64_t v=0;
        MU_ASSERT(bej_read_nnint(&br,&v)==1);
        MU_ASSERT(v==300);
    }
}

/* 2) словарь: root с одним ребёнком seq=1 ("Foo"), проверка lookup */
TEST(test_dict_load_lookup){
    uint8_t dict[256]; memset(dict,0,sizeof(dict));
    size_t n=0; uint8_t* p=dict;

    /* header */
    push_u8(&p,&n,0x01);                /* VersionTag */
    push_u8(&p,&n,0x00);                /* Flags */
    push_u16le(&p,&n,2);                /* EntryCount: root + 1 */
    push_u32le(&p,&n,0);                /* SchemaVersion */
    push_u32le(&p,&n,0);                /* DictionarySize */

    size_t entries_ofs = n;

    /* 2 entries * 10 bytes */
    for(int i=0;i<20;i++) push_u8(&p,&n,0x00);

    /* names pool */
    uint16_t off_root = (uint16_t)n; push_cstr(&p,&n,"Root");
    uint16_t off_foo  = (uint16_t)n; push_cstr(&p,&n,"Foo");

    /* fill entries */
    /* entry 0 (root) */
    {
        size_t q = entries_ofs + 0*10;
        dict[q+0] = 0x00;               /* fmt */
        dict[q+1] = 0x00; dict[q+2]=0;  /* seq=0 */
        uint16_t child_off = (uint16_t)(entries_ofs + 10);
        dict[q+3] = (uint8_t)(child_off & 0xFF);
        dict[q+4] = (uint8_t)((child_off>>8)&0xFF);
        dict[q+5] = 0x01; dict[q+6]=0x00; /* child_cnt=1 */
        dict[q+7] = 5;                    /* "Root" + NUL */
        dict[q+8] = (uint8_t)(off_root & 0xFF);
        dict[q+9] = (uint8_t)((off_root>>8)&0xFF);
    }
    /* entry 1 (child seq=1, name Foo) */
    {
        size_t q = entries_ofs + 1*10;
        dict[q+0] = 0x33;               /* fmt INT in dict (не критично) */
        dict[q+1] = 0x01; dict[q+2]=0;  /* seq=1 */
        dict[q+3] = 0x00; dict[q+4]=0;  /* no children */
        dict[q+5] = 0x00; dict[q+6]=0;
        dict[q+7] = 4;                   /* "Foo" + NUL */
        dict[q+8] = (uint8_t)(off_foo & 0xFF);
        dict[q+9] = (uint8_t)((off_foo>>8)&0xFF);
    }

    bej_dict D;
    MU_ASSERT(bej_dict_load(dict, n, &D)==1);
    bej_cluster rootc;
    rootc.start_idx = (uint32_t)(((size_t)D.ent[0].child_off - D.entries_ofs)/10u);
    rootc.count     = D.ent[0].child_cnt;
    MU_ASSERT(rootc.start_idx==1u);
    MU_ASSERT(rootc.count==1u);

    const bej_dict_entry* e = bej_cluster_lookup_seq(&D, rootc, 1);
    MU_ASSERT(e!=NULL);
    const char* nm = bej_dict_name_at(&D, e->name_off);
    MU_ASSERT(nm!=NULL && strcmp(nm,"Foo")==0);

    bej_dict_free(&D);
}

/* --------------------- runner --------------------- */
int main(void){
    int before;

    before = g_failures; RUN_TEST(test_nnint_basic);
    before = g_failures; RUN_TEST(test_dict_load_lookup);

    if(g_failures){
        fprintf(stderr, "\nFAILED: %d test(s)\n", g_failures);
        return 1;
    }
    fprintf(stdout, "\nAll tests passed.\n");
    return 0;
}
