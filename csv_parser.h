/**
 * This code is distributed under the license: MIT License
 * Originally written by Zhenrong WANG
 * mailto: zhenrongwang@live.com
 */

#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include <stdio.h>
#include <stdint.h>

#define TEST_ROUNDS         30
#define LIST_SIZE_STEP      4096

#define ERR_FILE_OPEN       -1
#define ERR_NULL_PTR        -3
#define ERR_MEM_ALLOC       -5
#define ERR_FILE_STAT       -7
#define ERR_MAP_FAILED      -9
#define ERR_LIST_INSERT     -11
#define ERR_UNMAP_FAILED    -13
#define ERR_ARR_GROW        -15
#define ERR_ARR_COPY        -17

struct matched_array {
    char *src_addr;
    size_t line_len;
    char *matched_line;
};

struct slist {
    char *matched_line;
    struct slist *next;
};

/* This Macro seems not helpful to the performance. */
#define SLIST_INSERT_NODE_MACRO(pp_head, str) ({ \
        int err_flag = 0; \
        if (!pp_head || !str) { \
            err_flag = -1; \
        } else { \
            struct slist *new_node = (struct slist *)malloc(sizeof(struct slist)); \
            if (!new_node) { \
                err_flag = 1; \
            } else { \
                new_node->matched_line = str; \
                struct slist *tmp = *pp_head; \
                *pp_head = new_node; \
                (!tmp) ? ({new_node->next = NULL;}) : ({new_node->next = tmp;}); \
            } \
            err_flag = 0; \
        } \
        err_flag; \
        })

#define KMP_CREATE_NEXT_ARRAY_NEW(ptr, len) ({ \
        size_t *next_array = NULL; \
        if(ptr && len >= 1) { \
            if ((next_array = (size_t *)calloc(len, sizeof(size_t))) != NULL) { \
                size_t i = 0, j = 1; \
                while (j < len) { \
                    if (ptr[j] == ptr[i]) { \
                        next_array[j] = next_array[j - 1] + 1; \
                        ++i; ++j; \
                    } else { \
                        if (!i) { next_array[j] = 0; ++j; } else { i = next_array[i - 1]; } \
                    } \
                } \
            } \
        } \
        next_array; \
        })

#define KMP_SEARCH_ULTRA(line, line_len, search_substr, substr_len, next_array) ({ \
        int64_t ret = -1; \
        if (line && search_substr && line_len >= substr_len && substr_len && next_array) { \
            size_t i = 0, j = 0; \
            while (i < line_len) { \
                if (line[i] == search_substr[j]) { \
                    ++i; ++j; \
                    if (j == substr_len) { ret = i - j; break; } \
                } else { \
                    if (j) { j = next_array[j - 1]; } \
                    else   { ++i; } \
                } \
            } \
        } \
        ret; \
        })

size_t *kmp_create_next_array(char* string);
size_t *kmp_create_next_array_new(char* ptr, size_t len);
int64_t kmp_search_std(char line[], char search_substr[]);
int64_t kmp_search_fast(char line[], char search_substr[], size_t next_array[]);
int64_t kmp_search_ultra(char line[], size_t line_len, char search_substr[], size_t substr_len, size_t next_array[]);
int slist_insert_node(struct slist **head, char *str);
int insert_matched_line(struct slist **head, char *src_ptr, size_t line_len);
int csv_parser(const char *data_file, char *search_kwd, struct slist **matched_list, size_t *matched_line_num);
int csv_parser_arr(const char *data_file, char *search_kwd, struct matched_array **matched_array, size_t *matched_line_num);

#endif