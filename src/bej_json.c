/**
 * @file bej_json.c
 * @brief Minimal pretty JSON writer (UTF-8). Not a full JSON library.
 */

#include "bej.h"

/** @brief Initialize a JSON writer around a FILE*. */
void bej_jw_init(bej_jsonw* j, FILE* f){ j->f=f; j->ind=0; j->need_comma=0; }

/** @brief Emit a newline and indentation spaces. */
void bej_jw_nl(bej_jsonw* j){ fputc('\n',j->f); for(int i=0;i<j->ind;i++) fputs("   ", j->f); }

/** @brief Begin a JSON object. */
void bej_jw_begin_obj(bej_jsonw* j){ fputs("{",j->f); j->ind++; j->need_comma=0; bej_jw_nl(j); }

/** @brief End a JSON object. */
void bej_jw_end_obj(bej_jsonw* j){ bej_jw_nl(j); j->ind--; fputs("}", j->f); j->need_comma=1; }

/** @brief Begin a JSON array. */
void bej_jw_begin_arr(bej_jsonw* j){ fputs("[",j->f); j->ind++; j->need_comma=0; }

/** @brief End a JSON array. */
void bej_jw_end_arr(bej_jsonw* j){ j->ind--; fputs("]", j->f); j->need_comma=1; }

/**
 * @brief Emit a JSON object key (with quoting/escaping) and prepare for a value.
 * @param j JSON writer.
 * @param k Null-terminated UTF-8 key.
 */
void bej_jw_key(bej_jsonw* j, const char* k){
    if(j->need_comma) fputs(",\n", j->f); else j->need_comma=1;
    for(int i=0;i<j->ind;i++) fputs("   ", j->f);
    fputc('"',j->f);
    for(const char* p=k;*p;p++){
        if(*p=='"'||*p=='\\'){ fputc('\\',j->f); fputc(*p,j->f); }
        else fputc(*p,j->f);
    }
    fputs("\": ", j->f);
}

/**
 * @brief Emit a JSON string value with basic escaping.
 * @param j JSON writer.
 * @param s Null-terminated UTF-8 string.
 */
void bej_jw_str(bej_jsonw* j, const char* s){
    fputc('"',j->f);
    for(const char* p=s;*p;p++){
        if(*p=='"'||*p=='\\'){ fputc('\\',j->f); fputc(*p,j->f); }
        else if(*p=='\n') fputs("\\n",j->f);
        else fputc(*p,j->f);
    }
    fputc('"',j->f);
}

/**
 * @brief Emit a JSON integer value.
 * @param j JSON writer.
 * @param v Signed integer.
 */
void bej_jw_int(bej_jsonw* j, long long v){ char buf[64]; snprintf(buf,sizeof(buf),"%lld",v); fputs(buf,j->f); }
