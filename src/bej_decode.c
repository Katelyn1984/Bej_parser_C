/**
 * @file bej_decode.c
 * @brief Minimal BEJ decoder (subset) per DMTF DSP0218 suitable for the provided example.
 *
 * This implementation:
 * - Parses a Redfish **schema dictionary** binary (Table 31) and exposes a map of
 *   sequence numbers to property names and child clusters.
 * - Decodes a BEJ **bejEncoding** stream (version, flags, schema class) followed by a top-level tuple.
 * - Supports value formats: **Set**, **Array**, **Integer**, **String**.
 *   - **Annotations** are **ignored/skipped** by design (per task requirement).
 *   - **Enum** values are rendered as strings (resolved via the dictionary options cluster).
 * - Emits pretty-printed JSON to the output file.
 *
 * @note This is a pragmatic subset intended to match the task's example.
 *       It does **not** implement every BEJ/Redfish type or all validation rules in DSP0218.
 */

#include <string.h>
#include <stdlib.h>
#include "bej.h"

/* ---- helpers to emit JSON for primitive values ---- */

static int decode_value_int(bej_jsonw* jw, bej_br* br, uint64_t L){
    long long v=0;
    for(uint64_t i=0;i<L;i++){
        uint8_t b; if(!bej_br_u8(br,&b)) return 0;
        v |= (long long)b << (8*i);
    }
    bej_jw_int(jw, v);
    return 1;
}

static int decode_value_string(bej_jsonw* jw, bej_br* br, uint64_t L){
    char* s=(char*)malloc((size_t)L+1); if(!s) return 0;
    if(!bej_br_get(br, (uint8_t*)s, (size_t)L)){ free(s); return 0; }
    s[L]=0;
    /* strip trailing NUL(s) for neat JSON */
    for(long i=(long)L-1; i>=0; --i){ if(s[i]==0) s[i]=0; else break; }
    bej_jw_str(jw, s);
    free(s);
    return 1;
}

/* ---- forward decl ---- */
static int decode_value_set(bej_jsonw* jw, bej_br* br, const bej_dict* D, bej_cluster this_cluster);

/**
 * @brief Decode a Set value: a count followed by that many tuples; emit a JSON object.
 *
 * @param jw JSON writer.
 * @param br Reader positioned at the Set value (expects a leading nnint count).
 * @param D  Parsed dictionary (for names and child clusters).
 * @param this_cluster The cluster that defines member sequence numbers for this Set.
 * @return 1 on success, 0 on malformed input.
 *
 * @note Annotations (S LSB bit set) are skipped entirely per task requirement.
 */
static int decode_value_set(bej_jsonw* jw, bej_br* br, const bej_dict* D, bej_cluster this_cluster){
    uint64_t count; if(!bej_read_nnint(br,&count)) return 0;
    bej_jw_begin_obj(jw);
    for(uint64_t i=0;i<count;i++){
        /* Sequence (nnint). LSB=1 indicates Annotation (skipped). */
        uint64_t S; if(!bej_read_nnint(br,&S)) return 0;
        int is_annotation = (S & 1u) ? 1: 0;
        uint16_t seq = (uint16_t)(S >> 1);

        /* Tuple format and payload length */
        uint8_t F; if(!bej_br_u8(br,&F)) return 0;
        uint8_t fmt = (uint8_t)(F >> 4);
        uint64_t L; if(!bej_read_nnint(br,&L)) return 0;

        if(is_annotation){
            /* Skip annotation payload completely */
            if(bej_br_left(br) < (int)L) return 0;
            br->p += (size_t)L;
            continue;
        }

        /* Resolve property name within this cluster */
        const bej_dict_entry* de = bej_cluster_lookup_seq(D, this_cluster, seq);
        const char* name = (de && de->name_off)? bej_dict_name_at(D, de->name_off): NULL;
        char tmp[32]; if(!name){ snprintf(tmp,sizeof(tmp),"seq_%u", (unsigned)seq); name = tmp; }

        /* Emit key and decode value by format */
        bej_jw_key(jw, name);
        if(fmt==BEJ_FMT_INT){
            if(!decode_value_int(jw, br, L)) return 0;
        }else if(fmt==BEJ_FMT_STRING){
            if(!decode_value_string(jw, br, L)) return 0;
        }else if(fmt==BEJ_FMT_SET){
            /* Descend into child cluster if known */
            bej_cluster childc = (bej_cluster){0,0};
            if(de && de->child_off){
                childc.start_idx = (uint32_t)(((size_t)de->child_off - D->entries_ofs)/10);
                childc.count     = de->child_cnt;
            }
            if(!decode_value_set(jw, br, D, childc)) return 0;
        }else if(fmt==BEJ_FMT_ARRAY){
            /* Arrays: count + element tuples (we print a flat JSON array of element values) */
            uint64_t cnt; if(!bej_read_nnint(br,&cnt)) return 0;
            bej_jw_begin_arr(jw);
            for(uint64_t k=0;k<cnt;k++){
                /* Read element tuple header */
                uint64_t Se; if(!bej_read_nnint(br,&Se)) return 0;
                uint8_t  Fe; if(!bej_br_u8(br,&Fe)) return 0;
                uint8_t  fmt_e = (uint8_t)(Fe>>4);
                uint64_t Le; if(!bej_read_nnint(br,&Le)) return 0;

                if(k>0) fputs(", ", jw->f);
                if(fmt_e==BEJ_FMT_INT){
                    if(!decode_value_int(jw, br, Le)) return 0;
                }else if(fmt_e==BEJ_FMT_STRING){
                    if(!decode_value_string(jw, br, Le)) return 0;
                }else{
                    /* Unsupported element formats are skipped as null */
                    if(bej_br_left(br) < (int)Le) return 0; br->p += (size_t)Le; fputs("null", jw->f);
                }
            }
            bej_jw_end_arr(jw);
        }else if(fmt==BEJ_FMT_ENUM){
            /* Map enum ordinal to its name via the entry's child cluster (render as JSON string) */
            bej_br val = *br;
            uint64_t opt_idx;
            if(!bej_read_nnint(&val, &opt_idx)) return 0;
            if(bej_br_left(br) < (int)L) return 0; br->p += (size_t)L;
            const char* optname = "EnumOption";
            if(de && de->child_off && de->child_cnt){
                bej_cluster enumc; enumc.start_idx = (uint32_t)(((size_t)de->child_off - D->entries_ofs)/10);
                enumc.count = de->child_cnt;
                const bej_dict_entry* opt = bej_cluster_lookup_seq(D, enumc, (uint16_t)opt_idx);
                if(opt && opt->name_off) optname = bej_dict_name_at(D, opt->name_off);
            }
            bej_jw_str(jw, optname);
        }else{
            /* Unsupported formats: skip payload and emit null */
            if(bej_br_left(br) < (int)L) return 0;
            br->p += (size_t)L;
            fputs("null", jw->f);
        }
    }
    bej_jw_end_obj(jw);
    return 1;
}

/**
 * @brief Decode a complete BEJ stream (bejEncoding + top-level tuple) and emit JSON.
 *
 * @param out FILE* for output JSON.
 * @param bej Pointer to start of BEJ-encoded data.
 * @param bej_n Length of the BEJ data.
 * @param D Parsed schema dictionary used for names and clusters.
 * @return 1 on success, 0 on malformed input.
 *
 * @note The function expects @p bej to begin with **bejEncoding** header:
 *       `version(4 LE)`, `flags(2 LE)`, `schemaClass(1)`, followed by a tuple.
 *       The top-level tuple is expected to be a **Set** whose members are emitted at JSON root.
 */
int bej_decode_to_json(FILE* out, const uint8_t* bej, size_t bej_n, const bej_dict* D){
    if(!out || !bej || !D) return 0;

    bej_br br; bej_br_init(&br, bej, bej_n);
    bej_jsonw jw; bej_jw_init(&jw, out);

    /* bejEncoding header */
    if(bej_br_left(&br) < 7) return 0;
    uint32_t ver; memcpy(&ver, br.d+br.p, 4); br.p+=4;
    uint16_t flags; memcpy(&flags, br.d+br.p, 2); br.p+=2;
    uint8_t schemaClass; if(!bej_br_u8(&br, &schemaClass)) return 0;
    (void)ver; (void)flags; (void)schemaClass;

    /* Root cluster (children of root entry 0) */
    bej_cluster rootc = (bej_cluster){0,0};
    if(D->n>0 && D->ent[0].child_off){
        rootc.start_idx = (uint32_t)(((size_t)D->ent[0].child_off - D->entries_ofs)/10);
        rootc.count     = D->ent[0].child_cnt;
    }

    /* Parse and require a top-level Set */
    uint64_t S; if(!bej_read_nnint(&br, &S)) return 0;
    uint8_t F; if(!bej_br_u8(&br,&F)) return 0;
    uint8_t fmt = (uint8_t)(F>>4);
    uint64_t L; if(!bej_read_nnint(&br, &L)) return 0;
    if(fmt != BEJ_FMT_SET) return 0;

    /* Decode the top-level Set (decode_value_set writes the object braces) */
    if(!decode_value_set(&jw, &br, D, rootc)) return 0;
    fputc('\n', jw.f);
    return 1;
}
