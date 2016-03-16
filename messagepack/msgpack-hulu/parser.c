/*************************************************************************
    > File Name: parse_msg.c
    > Author: shuaixiang
    > Mail: shuaixiang@yuewen.com
    > Created Time: Fri 11 Mar 2016 03:24:11 AM UTC
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR_LEN 100

typedef enum {
    MSGPACK_OBJECT_NIL                  = 0x00,
    MSGPACK_OBJECT_BOOLEAN              = 0x01,
    MSGPACK_OBJECT_POSITIVE_INTEGER     = 0x02,
    MSGPACK_OBJECT_NEGATIVE_INTEGER     = 0x03,
    MSGPACK_OBJECT_FLOAT                = 0x04,
#if defined(MSGPACK_USE_LEGACY_NAME_AS_FLOAT)
    MSGPACK_OBJECT_DOUBLE               = MSGPACK_OBJECT_FLOAT, /* obsolete */
#endif /* MSGPACK_USE_LEGACY_NAME_AS_FLOAT */
    MSGPACK_OBJECT_STR                  = 0x05,
    MSGPACK_OBJECT_ARRAY                = 0x06,
    MSGPACK_OBJECT_MAP                  = 0x07,
    MSGPACK_OBJECT_BIN                  = 0x08,
    MSGPACK_OBJECT_EXT                  = 0x09
} msgpack_object_type;

typedef unsigned char ubyte_t ;
typedef char * string_t;
typedef int bool_t;
typedef struct Object{
    bool_t isKey;
    msgpack_object_type type;
    struct Object *next;
    struct Object *child;
    string_t str_val;
    int int_val;
    float float_val;
    string_t name;
} Object;

typedef struct Context{
    Object *root;
    Object *node;
    ubyte_t *buf;
    int off;
}Context; 

Object * NewObject(){
    Object *new_node = (Object *)calloc(1, sizeof(Object));
    if(NULL == new_node){
        perror("NewObject:");
        return NULL;
    }

    return new_node;
}

void ParseDispatcher(Context *ctx);

void ParseEXT(Context *ctx){/*{{{*/
    ubyte_t *index = ctx->buf + ctx->off;
    int *off = &ctx->off;
    ubyte_t head = *index & 0xFF;
    int len = 0;

    if(head == 0xC7){
        len = *(index + 1); 
        *off += 3;
        int start = *off;
        Object *parent = ctx->node;

        while((*off - start) < len){
            Object *new_node = NewObject();
            if(*off == start)
                parent->child = new_node;
            else
                ctx->node->next = new_node;
            ctx->node = new_node;
            ctx->node->isKey = 1; 
            ParseDispatcher(ctx);//key
            ctx->node->isKey = 0; 
            ParseDispatcher(ctx);//value
        }
    }
    else{
        printf("0x%x unimplement\n", head);
    }
   
}/*}}}*/

void ParseMap(Context *ctx){/*{{{*/
    ubyte_t *index = ctx->buf + ctx->off;
    int *off = &ctx->off;
    ubyte_t head = *index & 0xFF;
    int len = 0, i = 0;
    
    if((head & 0xF0) == 0x80){
        len = 0x0F & head;
        *off += 1;
        Object *parent = ctx->node;
        for(i = 0; i < len; i++){
            Object *new_node = NewObject();
            if(0 == i)
                parent->child = new_node;
            else
                ctx->node->next = new_node;
            ctx->node = new_node;
            ctx->node->isKey = 1;
            ParseDispatcher(ctx);//key
            ctx->node->isKey = 0;
            ParseDispatcher(ctx);//value
        }
    }

    if(head == 0xDE){
        printf("0xde unimplement\n");
    }

    if(head == 0xDF){
        printf("0xdf unimplement\n");
    }
}/*}}}*/

void ParseArray(Context *ctx){/*{{{*/
    ubyte_t *index = ctx->buf + ctx->off;
    int *off = &ctx->off;
    ubyte_t head = *index & 0xFF;
    int len = 0, i = 0;

    if((head & 0xF0) == 0x90){
        len = 0x0F & head;
        *off += 1;
        Object *parent = ctx->node;
        for(i = 0; i < len; i++){
            Object *new_node = NewObject();
            if(0 == i)
                parent->child = new_node;
            else
                ctx->node->next = new_node;
            ctx->node = new_node;
            ParseDispatcher(ctx);
        }
    }
    if(head == 0xDC){
        printf("0xdc unimplement\n");
    }
    if(head == 0xDD){
        printf("0xdd unimplement\n");
    }
}/*}}}*/

ubyte_t *ParseBin(Context *ctx){/*{{{*/
    ubyte_t *index = ctx->buf + ctx->off;
    int *off = &ctx->off;
    ubyte_t head = *index & 0xFF;
    ubyte_t *bin= NULL;
    int len = 0;
    
    switch(head){
        case 0xC4: 
            len = *(index+1);
            printf("len: %d\n", len);
            bin = (ubyte_t *) malloc(sizeof(ubyte_t)*len);
            memcpy(bin, (index+ 2), len);
            *off += (len + 1);
            return bin;
        case 0xC5:
            len = (*(index+1)<<8) + *(index+2);
            printf("len: %d\n", len);
            bin = (ubyte_t *) malloc(sizeof(ubyte_t)*len);
            memcpy(bin, (index+3), len);
            *off += (len + 1);
            return bin;
        case 0xC6:
            len = (*(index + 1)<<24) + (*(index + 2)<<16) + (*(index + 3)<<8) + *(index + 4);
            printf("len: %d\n", len);
            bin = (ubyte_t*) malloc(sizeof(ubyte_t)*len);
            memcpy(bin, (index+5), len);
            *off += (len+ 1);
            return bin;
    }

    return bin;
}/*}}}*/

void ParseString(Context *ctx){/*{{{*/
    ubyte_t *index = ctx->buf + ctx->off;
    int *off = &ctx->off;
    ubyte_t head = *index & 0xFF;
    string_t str = NULL;
    int len = 0;

    if ((head & 0xE0) == 0xA0){
        len = head &0x1F;
        printf("len: %d\n", len);
        str = (char *) malloc(sizeof(char)*len);
        memcpy(str, (index + 1), len);
        *off += (len + 1);
        if(ctx->node->isKey == 0){
            ctx->node->str_val = str;
        }else{
            ctx->node->name = str;
        }
        return;
    }
    
    switch(head){
        case 0xD9: 
            len = *(index+1);
            printf("len: %d\n", len);
            str = (char *) malloc(sizeof(char)*len);
            memcpy(str, (index + 2), len);
            *off += (len + 1);
            if(ctx->node->isKey == 0){
                ctx->node->str_val = str;
            }else{
                ctx->node->name = str;
            }
            return;
        case 0xDA:
            len = (*(index + 1)<<8) + *(index+2);
            printf("len: %d\n", len);
            str = (char *) malloc(sizeof(char)*len);
            memcpy(str, (index + 3), len);
            *off += (len + 1);
            if(ctx->node->isKey == 0){
                ctx->node->str_val = str;
            }else{
                ctx->node->name = str;
            }
            return;
        case 0xDB:
            len = (*(index + 1)<<24) + (*(index + 2)<<16) + (*(index + 3)<<8) + *(index + 4);
            printf("len: %d\n", len);
            str = (char *) malloc(sizeof(char)*len);
            memcpy(str, (index + 5), len);
            *off += (len+ 1);
            if(ctx->node->isKey == 0){
                ctx->node->str_val = str;
            }else{
                ctx->node->name = str;
            }
            return;
    }
}/*}}}*/

int ParseBool(Context *ctx){/*{{{*/
    ubyte_t *index = ctx->buf + ctx->off;
    int *off = &ctx->off;
    ubyte_t head = *index;
    *off +=  1;
    if(head == 0xC2)
        return 0;
    else if(head == 0xC3)
        return 1;
    else
        return -1;
}/*}}}*/

void *ParseNil(Context *ctx){/*{{{*/
    ubyte_t *index = ctx->buf + ctx->off;
    int *off = &ctx->off;
    ubyte_t head = *index;
    *off += 1;
    if(head == 0xC0)
        return NULL;
    return NULL;
}/*}}}*/

void ParseInteger(Context *ctx){/*{{{*/
    ubyte_t *index = ctx->buf + ctx->off;
    string_t str_int = NULL; 
    char tmp_str[STR_LEN] = {0};
    int str_len = 0;
    int *off = &ctx->off;
    ubyte_t head = *index & 0xFF;
    unsigned int uval_8 = 0, uval_16 = 0, uval_32 = 0;
    unsigned long long uval_64 = 0;
    signed int val_8 = 0, val_16 = 0, val_32 = 0;
    long long val_64 = 0;
    unsigned long long tmp = 0;
    int i = 0;
    
    if( (head & 0x80) == 0){
        int val = (*index) & 0x7F; 
        *off += 1;
        if(ctx->node->isKey == 0){
            ctx->node->int_val=val;
        }
        else{
            sprintf(tmp_str, "%d", val);
            str_len = strlen(tmp_str);
            str_int = (string_t) malloc(sizeof(char)*str_len);
            memcpy(str_int, tmp_str, str_len);
            ctx->node->name = str_int;
        }
        return;
    }

    if( (head & 0xE0) == 0xE0){
        int val = (*index) & 0x1F;
        *off += 1;
        if(ctx->node->isKey == 0){
            ctx->node->int_val= -val;
        }
        else{
            sprintf(tmp_str, "%d", -val);
            str_len = strlen(tmp_str);
            str_int = (string_t) malloc(sizeof(char)*str_len);
            memcpy(str_int, tmp_str, str_len);
            ctx->node->name = str_int;
        }
       return;
    }

    switch(head){
        case 0xCC:
            uval_8 = (unsigned int) *(index + 1);
            printf("val: %u\n", uval_8);
            *off += 2;
            if(ctx->node->isKey == 0){
                ctx->node->int_val = uval_8;
            }
            else{
                sprintf(tmp_str, "%u", uval_8);
                str_len = strlen(tmp_str);
                str_int = (string_t) malloc(sizeof(char)*(str_len));
                memcpy(str_int, tmp_str, str_len);
                ctx->node->name = str_int;
            }
            return;
            
        case 0xCD:
            uval_16 = (unsigned int)(*(index + 1) << 8) + *(index + 2);
            printf("val: %u\n", uval_16);
            *off += 3;
            if(ctx->node->isKey == 0){
                ctx->node->int_val = uval_16;
            }
            else{
                sprintf(tmp_str, "%u", uval_16);
                str_len = strlen(tmp_str);
                str_int = (string_t) malloc(sizeof(char)*str_len);
                memcpy(str_int, tmp_str, str_len);
                ctx->node->name = str_int;
            }
            return;
            
        case 0xCE:
            uval_32 = (unsigned int)((*(index + 1) << 24) + (*(index + 2) << 16) + (*(index + 3) << 8) + *(index + 4));
            printf("val: %u\n", uval_32);
            *off += 5;
            if(ctx->node->isKey == 0){
                ctx->node->int_val = uval_32;
            }
            else{
                sprintf(tmp_str, "%u", uval_32);
                str_len = strlen(tmp_str);
                str_int = (string_t) malloc(sizeof(char)*str_len);
                memcpy(str_int, tmp_str, str_len);
                ctx->node->name = str_int;
            }
            return;

        case 0xCF:
            tmp = *(index + 1);
            for(i = 0; i < 8; i++){
                tmp = *(index + 8 - i );
                uval_64 += tmp << (i * 8);
            }
            printf("val: %llu\n", uval_64);
            *off += 9;
            if(ctx->node->isKey == 0){
                ctx->node->int_val = uval_64;
            }
            else{
                sprintf(tmp_str, "%llu", uval_64);
                str_len = strlen(tmp_str);
                str_int = (string_t) malloc(sizeof(char)*(str_len));
                memcpy(str_int, tmp_str, str_len);
                ctx->node->name = str_int;
            }
            return;

        case 0xD0:
            val_8 = (int) *(index + 1);
            printf("val: %d\n", val_8);
            *off += 2;
            if(ctx->node->isKey == 0){
                ctx->node->int_val = val_8;
            }
            else{
                sprintf(tmp_str, "%d", val_8);
                str_len = strlen(tmp_str);
                str_int = (string_t) malloc(sizeof(char)*str_len);
                memcpy(str_int, tmp_str, str_len);
                ctx->node->name = str_int;
            }
            return;

        case 0xD1:
            val_16 = (int) ((*(index + 1) << 8) + *(index + 2));
            printf("val: %d\n", val_16);
            *off += 3;
            if(ctx->node->isKey == 0){
                ctx->node->int_val = val_16;
            }
            else{
                sprintf(tmp_str, "%d", val_16);
                str_len = strlen(tmp_str);
                str_int = (string_t) malloc(sizeof(char)*str_len);
                memcpy(str_int, tmp_str, str_len);
                ctx->node->name = str_int;
            }
            return;

        case 0xD2:
            val_32 = (int) ((*(index + 1) << 24) + (*(index + 2) << 16) + (*(index + 3) << 8) + *(index + 4));
            printf("val: %d\n", val_32);
            *off += 5;
            if(ctx->node->isKey == 0){
                ctx->node->int_val = val_32;
            }
            else{
                sprintf(tmp_str, "%d", val_32);
                str_len = strlen(tmp_str);
                str_int = (string_t) malloc(sizeof(char)*str_len);
                memcpy(str_int, tmp_str, str_len);
                ctx->node->name = str_int;
            }
            return;

        case 0xD3:
            tmp = *(index + 1);
            for(i = 0; i < 8; i++){
                tmp = *(index + 8 - i );
                val_64 += tmp << (i * 8);
            }
            printf("val: %lld\n", val_64);
            *off += 9;
            if(ctx->node->isKey == 0){
                ctx->node->int_val = val_64;
            }
            else{
                sprintf(tmp_str, "%lld", val_64);
                str_len = strlen(tmp_str);
                str_int = (string_t) malloc(sizeof(char)*str_len);
                memcpy(str_int, tmp_str, str_len);
                ctx->node->name = str_int;
            }
            return;
    }
}/*}}}*/

void ParseFloat(Context *ctx){/*{{{*/
    ubyte_t *index = ctx->buf + ctx->off;
    int *off = &ctx->off;
    ubyte_t head = *index;
    union{
        float float_tmp;
        ubyte_t float_buf[4]; 
        double double_tmp;
        ubyte_t double_buf[8];
    } float_union;

    if(head == 0xCA){
        int i = 0;
        for(i = 0; i < 4; i++){
            float_union.float_buf[i] = *(index + 4 - i);  
        }
        *off += 5;
        ctx->node->float_val = float_union.float_tmp;
        return;
    }
    if(head == 0xCB){
        int i  = 0;
        for(i = 1; i<=8; i++){
            float_union.double_buf[i] = *(index + 8 - i);
        }
        *off += 9;
        ctx->node->float_val = float_union.double_tmp;
        return;
    }
}/*}}}*/

void  ParseDispatcher(Context *ctx){/*{{{*/
    ubyte_t *index = ctx->buf + ctx->off;
    ubyte_t head = *index;
    if ( (head & 0x80) == 0){
        ctx->node->type = MSGPACK_OBJECT_NEGATIVE_INTEGER;
        ParseInteger(ctx);
        printf("positive fixint\n");
        return;
    }
    if( (head & 0xE0) == 0xE0){
        printf("negative fixint\n");
        return;
    }
    if( (head & 0xE0) == 0xA0){
        ctx->node->type = MSGPACK_OBJECT_STR;
        ParseString(ctx);
        printf("fixRaw\n");
        return;
    }
    if( (head & 0xF0) == 0x90){
        ctx->node->type = MSGPACK_OBJECT_ARRAY;
        ParseArray(ctx);
        printf("fixarray\n");
        return;
    }
    if( (head & 0xF0) == 0x80){
        ctx->node->type = MSGPACK_OBJECT_MAP;
        ParseMap(ctx);
        printf("FixMap\n");
        return;
    }

    switch(head & 0xFF){
        case 0xC0:
            ctx->node->type = MSGPACK_OBJECT_NIL;
            ParseNil(ctx);
            printf("NIL\n");
            return;
        case 0xC2:
            ctx->node->type = MSGPACK_OBJECT_BOOLEAN;
            ParseBool(ctx);
            printf("boolean false\n");
            return;
        case 0xC3:
            ctx->node->type = MSGPACK_OBJECT_BOOLEAN;
            ParseBool(ctx);
            printf("boolean true\n");
            return;
        case 0xCA:
            ctx->node->type = MSGPACK_OBJECT_FLOAT;
            ParseFloat(ctx);
            printf("float\n");
            return;
        case 0xCB:
            ctx->node->type = MSGPACK_OBJECT_FLOAT;
            ParseFloat(ctx);
            printf("DOUBLE\n");
            return;
        case 0xCC:
            ctx->node->type = MSGPACK_OBJECT_NEGATIVE_INTEGER;
            ParseInteger(ctx);
            printf("unsigned int 8\n");
            return;
        case 0xCD:
            ctx->node->type = MSGPACK_OBJECT_NEGATIVE_INTEGER;
            ParseInteger(ctx);
            printf("unsigned int 16\n");
            return;
        case 0xCE:
            ctx->node->type = MSGPACK_OBJECT_NEGATIVE_INTEGER;
            ParseInteger(ctx);
            printf("unsigned int 32\n");
            return;
        case 0xCF:
            ctx->node->type = MSGPACK_OBJECT_NEGATIVE_INTEGER;
            ParseInteger(ctx);
            printf("unsigned int 64\n"); 
            return;
        case 0xD0:
            ctx->node->type = MSGPACK_OBJECT_NEGATIVE_INTEGER;
            ParseInteger(ctx);
            printf("signed int 8\n");
            return;
        case 0xD1:
            ctx->node->type = MSGPACK_OBJECT_NEGATIVE_INTEGER;
            ParseInteger(ctx);
            printf("signed int 16\n");
            return;
        case 0xD2:
            ctx->node->type = MSGPACK_OBJECT_NEGATIVE_INTEGER;
            ParseInteger(ctx);
            printf("signed int 32\n");
            return;
        case 0xD3:
            ctx->node->type = MSGPACK_OBJECT_NEGATIVE_INTEGER;
            ParseInteger(ctx);
            printf("signed int 64\n");
            return;
        case 0xC4:
            ctx->node->type = MSGPACK_OBJECT_BIN;
            ParseBin(ctx);
            printf("bin 8\n");
            return;
        case 0xC5:
            ctx->node->type = MSGPACK_OBJECT_BIN;
            ParseBin(ctx);
            printf("bin 16\n");
            return;
        case 0xC6:
            ctx->node->type = MSGPACK_OBJECT_BIN;
            ParseBin(ctx);
            printf("bin 32\n");
            return;
        case 0xD9:
            ctx->node->type = MSGPACK_OBJECT_STR;
            ParseString(ctx);
            printf("str8\n");
            return;
        case 0xDA:
            ctx->node->type = MSGPACK_OBJECT_STR;
            ParseString(ctx);
            printf("str 16\n");
            return;
        case 0xDB:
            ctx->node->type = MSGPACK_OBJECT_STR;
            ParseString(ctx);
            printf("str 32\n");
            return;
        case 0xDC:
            ctx->node->type = MSGPACK_OBJECT_ARRAY;
            ParseArray(ctx);
            printf("array 16\n");
            return;
        case 0xDD:
            ctx->node->type = MSGPACK_OBJECT_ARRAY;
            ParseArray(ctx);
            printf("array 32\n");
            return;
        case 0xDE:
            printf("map 16\n");
            return;
        case 0xDF:
            ctx->node->type = MSGPACK_OBJECT_MAP;
            ParseMap(ctx);
            printf("map 32\n");
            return;
        case 0xC7:
            ctx->node->type = MSGPACK_OBJECT_EXT;
            ParseEXT(ctx);
            printf("ext 8\n");
        default:
            printf("not found\n");
    }
}/*}}}*/

int main(){
    ubyte_t ext[] = {0xC7, 0x11, 0x00, 0xA4, 0x6E, 0x61, 0x6D, 0x65, 0xA6, 0xE5, 0x91, 0xB5, 0xE5, 0x91, 0xB5, 0xA3, 0x61, 0x67, 0x65, 0x0A};
    ubyte_t str[] = {0xD9, 0x29, 0x61, 0x62, 0x63, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6D, 0x6D, 0x6D, 0x6D, 0x6D, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73,  0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73};
    ubyte_t array[] = {0x93, 0x01, 0x02, 0x03};
    ubyte_t float_val[] = {0xCA, 0x43, 0x5C, 0x00, 0x00};
    ubyte_t map[] = {0x82, 0x02, 0xA5, 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x03, 0xA5, 0x77, 0x6F, 0x72, 0x6C, 0x64};
    
    //char *ret_str = parseString(str, &off);
    //printf("%s\n", ret_str);
    Context *ctx = (Context *)calloc(1, sizeof(Context));
    ctx->root = NewObject(); 
    ctx->node = ctx->root;
    //ctx->buf = array;
    //ctx->buf = float_val;
    //ctx->buf = ext;
    ctx->buf = float_val;
    ParseDispatcher(ctx);

    //ctx->buf = float_val;
    //printf("%f\n", ParseFloat(ctx));
}

