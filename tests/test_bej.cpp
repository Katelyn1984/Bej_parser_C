#include <gtest/gtest.h>
#include <vector>
#include <cstdint>
#include <cstring>

extern "C" {
#include "bej.h"
}

// tiny helpers
static void push_u8(std::vector<uint8_t>& v, uint8_t x){ v.push_back(x); }
static void push_u16le(std::vector<uint8_t>& v, uint16_t x){
    v.push_back(uint8_t(x & 0xFF)); v.push_back(uint8_t((x>>8) & 0xFF));
}
static void push_u32le(std::vector<uint8_t>& v, uint32_t x){
    v.push_back(uint8_t(x & 0xFF));
    v.push_back(uint8_t((x>>8) & 0xFF));
    v.push_back(uint8_t((x>>16) & 0xFF));
    v.push_back(uint8_t((x>>24) & 0xFF));
}
static void push_nnint(std::vector<uint8_t>& v, uint64_t val){
    uint8_t buf[8]{0}; int N=0;
    if(val==0){ N=1; buf[0]=0; }
    else { while(val){ buf[N++]=uint8_t(val&0xFF); val>>=8; } }
    v.push_back(uint8_t(N)); for(int i=0;i<N;i++) v.push_back(buf[i]);
}
static void push_cstr(std::vector<uint8_t>& v, const char* s){
    while(*s) v.push_back(uint8_t(*s++)); v.push_back(0);
}

// ---- tests ----

// 1) nnint: два граничных значения
TEST(Min, NNIntBasic){
    {
        std::vector<uint8_t> b = {0x01,0x00}; // 0
        bej_br br; bej_br_init(&br, b.data(), b.size());
        uint64_t val=123; ASSERT_TRUE(bej_read_nnint(&br,&val)); EXPECT_EQ(val, 0u);
        EXPECT_EQ(bej_br_left(&br), 0);
    }
    {
        std::vector<uint8_t> b = {0x02,0x2C,0x01}; // 300
        bej_br br; bej_br_init(&br, b.data(), b.size());
        uint64_t val=0; ASSERT_TRUE(bej_read_nnint(&br,&val)); EXPECT_EQ(val, 300u);
    }
}

// 2) словарь: root->(seq=1:"Foo"), lookup по кластеру
TEST(Min, DictLoadAndLookup){
    std::vector<uint8_t> dict;

    // header
    push_u8(dict, 0x01);            // VersionTag
    push_u8(dict, 0x00);            // Flags
    push_u16le(dict, 2);            // EntryCount: root + 1 child
    push_u32le(dict, 0);            // SchemaVersion
    push_u32le(dict, 0);            // DictionarySize

    size_t entries_ofs = dict.size();
    // reserve 2 entries * 10 bytes
    for(int i=0;i<20;i++) dict.push_back(0);

    // names pool
    size_t names_ofs = dict.size();
    uint16_t off_root = (uint16_t)names_ofs; push_cstr(dict, "Root");
    uint16_t off_foo  = (uint16_t)dict.size(); push_cstr(dict, "Foo");

    auto set_entry = [&](int idx, uint8_t fmt, uint16_t seq, uint16_t child_off,
                         uint16_t child_cnt, uint8_t name_len, uint16_t name_off){
        size_t p = entries_ofs + idx*10;
        dict[p+0]=fmt;
        dict[p+1]=uint8_t(seq&0xFF); dict[p+2]=uint8_t((seq>>8)&0xFF);
        dict[p+3]=uint8_t(child_off&0xFF); dict[p+4]=uint8_t((child_off>>8)&0xFF);
        dict[p+5]=uint8_t(child_cnt&0xFF); dict[p+6]=uint8_t((child_cnt>>8)&0xFF);
        dict[p+7]=name_len;
        dict[p+8]=uint8_t(name_off&0xFF); dict[p+9]=uint8_t((name_off>>8)&0xFF);
    };

    uint16_t child_off_root = (uint16_t)(entries_ofs + 10);
    set_entry(0, /*fmt*/0x00, /*seq*/0, child_off_root, /*child_cnt*/1, /*len*/5, off_root);
    set_entry(1, /*fmt*/0x33, /*seq*/1, 0, 0, /*len*/4, off_foo);

    bej_dict D{};
    ASSERT_TRUE(bej_dict_load(dict.data(), dict.size(), &D));
    bej_cluster rootc {
        (uint32_t)(((size_t)D.ent[0].child_off - D.entries_ofs)/10),
        D.ent[0].child_cnt
    };
    ASSERT_EQ(rootc.start_idx, 1u);
    ASSERT_EQ(rootc.count, 1u);

    auto* e = bej_cluster_lookup_seq(&D, rootc, 1);
    ASSERT_NE(e, nullptr);
    EXPECT_STREQ(bej_dict_name_at(&D, e->name_off), "Foo");

    bej_dict_free(&D);
}
