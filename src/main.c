/**
 * @file main.c
 * @brief CLI entrypoint: load files, decode BEJ to JSON using schema dictionary.
 *
 * Usage:
 *   bej_tool -s <schema.bin> -a <annotation.bin> -b <data.bej> -o <out.json>
 * Note: Annotation dictionary is opened/ignored. Supported: Set, Array, Int, String; Enum→String.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bej.h"

/* Simple file loader */
static int load_file(const char* path, uint8_t** out, size_t* out_n){
    *out=NULL; *out_n=0; FILE* f=fopen(path,"rb"); if(!f) return 0;
    fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
    if(sz<=0){ fclose(f); return 0; }
    uint8_t* buf=(uint8_t*)malloc((size_t)sz);
    if(!buf){ fclose(f); return 0; }
    if(fread(buf,1,(size_t)sz,f)!=(size_t)sz){ free(buf); fclose(f); return 0; }
    fclose(f); *out=buf; *out_n=(size_t)sz; return 1;
}

static void usage(const char* a0){
    fprintf(stderr,
        "Usage: %s -s <schema.bin> -a <annotation.bin> -b <data.bej> -o <out.json>\n"
        "Note: Annotation dictionary is opened/ignored. Supported: Set, Array, Int, String; Enum->String.\n", a0);
}

int main(int argc, char** argv){
    const char* sp=NULL; const char* ap=NULL; const char* bp=NULL; const char* op=NULL;
    for(int i=1;i<argc;i++){
        if(strcmp(argv[i],"-s")==0 && i+1<argc) sp=argv[++i];
        else if(strcmp(argv[i],"-a")==0 && i+1<argc) ap=argv[++i];
        else if(strcmp(argv[i],"-b")==0 && i+1<argc) bp=argv[++i];
        else if(strcmp(argv[i],"-o")==0 && i+1<argc) op=argv[++i];
        else { usage(argv[0]); return 1; }
    }
    if(!sp||!ap||!bp||!op){ usage(argv[0]); return 1; }

    uint8_t *sbuf=NULL,*bbuf=NULL; size_t sn=0,bn=0;
    if(!load_file(sp,&sbuf,&sn)){ fprintf(stderr,"ERROR: open schema %s\n", sp); return 2; }
    FILE* fa=fopen(ap,"rb"); if(!fa){ fprintf(stderr,"ERROR: open annotation %s\n", ap); free(sbuf); return 3; } fclose(fa);
    if(!load_file(bp,&bbuf,&bn)){ fprintf(stderr,"ERROR: open bej %s\n", bp); free(sbuf); return 4; }

    bej_dict D; if(!bej_dict_load(sbuf,sn,&D)){ fprintf(stderr,"ERROR: parse schema dict\n"); free(sbuf); free(bbuf); return 5; }

    FILE* fo=fopen(op,"wb"); if(!fo){ fprintf(stderr,"ERROR: open out %s\n", op); free(sbuf); free(bbuf); bej_dict_free(&D); return 6; }
    int ok = bej_decode_to_json(fo, bbuf, bn, &D);
    fclose(fo);
    free(sbuf); free(bbuf); bej_dict_free(&D);
    if(!ok){ fprintf(stderr,"ERROR: decode\n"); remove(op); return 7; }
    return 0;
}
