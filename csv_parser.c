/**
 * This code is distributed under the license: MIT License
 * Originally written by Zhenrong WANG
 * mailto: zhenrongwang@live.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "csv_parser.h"

#ifdef _WIN32
    #include <Windows.h>
    #include <Memoryapi.h>
#else 
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/mman.h>
#endif

/* KMP algorithm preprocessor: create NEXT array. */
size_t *kmp_create_next_array(char* string) {
    size_t i = 0, j = 1, length = 0;
    if (!string || (length = strlen(string)) < 1) 
        return NULL;
    size_t *next_array = (size_t *)calloc(length, sizeof(size_t));
    if (!next_array) 
        return NULL;
    while (j < length) {
        if (string[j] == string[i]) {
            next_array[j] = next_array[j - 1] + 1;
            i++; j++;
        }
        else {
            if (!i) { next_array[j] = 0; j++; }
            else { i = next_array[i - 1]; }
        }
    }
    return next_array;
}

/* KMP algorithm preprocessor: create NEXT array. */
size_t *kmp_create_next_array_new(char* ptr, size_t len) {
    if (!ptr || len < 1) 
        return NULL;
    size_t *next_array = (size_t *)calloc(len, sizeof(size_t));
    if (!next_array) 
        return NULL;
    size_t i = 0, j = 1;
    while (j < len) {
        if (ptr[j] == ptr[i]) {
            next_array[j] = next_array[j - 1] + 1;
            ++i; ++j;
        }
        else {
            if (!i) { next_array[j] = 0; ++j; }
            else { i = next_array[i - 1]; }
        }
    }
    return next_array;
}

/* The standard KMP search algorithm. It frees the NEXT array. */
int64_t kmp_search_std(char line[], char search_substr[]) {
    if (!line || !search_substr) 
        return -1;
    size_t line_len = strlen(line), key_len = strlen(search_substr), i = 0, j = 0;
    if (line_len < key_len || key_len == 0) 
        return -1;
    size_t* next_array = kmp_create_next_array(search_substr);
    if (!next_array)
        return -1;
    while (i < line_len) {
        if (line[i] == search_substr[j]) {
            ++i; ++j;
            if (j == key_len) {
                free(next_array);
                return i - j;
            }
        }
        else {
            if (j) { j = next_array[j - 1]; }
            else   { ++i; }
        }
    }
    free(next_array); // NEXT array freed.
    return -1;
}

/* Fast version: the NEXT array is managed outside of this function, 
 * The NEXT array MUST be generated by the search_substr ! */
int64_t kmp_search_fast(char line[], char search_substr[], size_t next_array[]) {
    if (!line || !search_substr) 
        return -1;
    size_t line_len = strlen(line), key_len = strlen(search_substr), i = 0, j = 0;
    if (line_len < key_len || key_len == 0) 
        return -1;
    if (!next_array)
        return -1;
    while (i < line_len) {
        if (line[i] == search_substr[j]) {
            ++i; ++j;
            if (j == key_len)
                return i - j;
        }
        else {
            if (j) { j = next_array[j - 1]; }
            else   { ++i; }
        }
    }
    return -1;
}

/* Ultra version: the NEXT array is managed outside of this function,
 * Put line_len and substr_len as arguments because they are known for the call 
 * The NEXT array MUST be generated by the search_substr ! */
int64_t kmp_search_ultra(char line[], size_t line_len, char search_substr[], size_t substr_len, size_t next_array[]) {
    if (!line || !search_substr) 
        return -1;
    if (line_len < substr_len || !substr_len) 
        return -1;
    if (!next_array)
        return -1;
    size_t i = 0, j = 0;
    while (i < line_len) {
        if (line[i] == search_substr[j]) {
            ++i; ++j;
            if (j == substr_len)
                return i - j;
        }
        else {
            if (j) { j = next_array[j - 1]; }
            else   { ++i; }
        }
    }
    return -1;
}  

/* This function is good to go, but I replaced it with the "insert_matched_line" below. */
int slist_insert_node(struct slist **head, char *str) {
    if(!head || !str) 
        return -1;
    struct slist *new_node = (struct slist *)malloc(sizeof(struct slist));
    if(!new_node)
        return 1;
    new_node->matched_line = str;
    struct slist *tmp = *head;
    *head = new_node;
    if(!tmp) 
        new_node->next = NULL;
    else
        new_node->next = tmp;
    return 0;
}

int insert_matched_line(struct slist **head, char *src_ptr, size_t line_len) {
    if(!head || !src_ptr) 
        return -1;
    struct slist *new_node = (struct slist *)malloc(sizeof(struct slist));
    if(!new_node)
        return 1;
    new_node->matched_line = (char *)malloc((line_len + 1) * sizeof(char));
    if(!(new_node->matched_line)) {
        free(new_node);
        return 3;
    }
    memcpy(new_node->matched_line, src_ptr, line_len);
    new_node->matched_line[line_len] = '\0';
    struct slist *tmp = *head;
    *head = new_node;
    if(!tmp) 
        new_node->next = NULL;
    else
        new_node->next = tmp;
    return 0;
}

/**
 * Parse the CSV file and find the lines with the "search_kwd". 
 */
int csv_parser(const char *data_file, char *search_kwd, struct slist **matched_list, size_t *matched_line_num) {
    int ret_flag = 0;
#ifndef _WIN32
    int fd = open(data_file, O_RDONLY);
    if(fd == -1) {
        ret_flag = ERR_FILE_OPEN;
        goto clean_and_return;
    }
#endif
    if(!search_kwd || !matched_list || !matched_line_num) {
        ret_flag = ERR_NULL_PTR;
        goto clean_and_return;
    }
    size_t kwd_len = strlen(search_kwd);
    size_t *next_array = kmp_create_next_array_new(search_kwd, kwd_len);
    if(!next_array) {
        ret_flag = ERR_MEM_ALLOC;
        goto clean_and_return;
    }
 #ifndef _WIN32
    struct stat file_stat;
    if(fstat(fd, &file_stat) == -1) {
        ret_flag = ERR_FILE_STAT;
        goto clean_and_return;
    }
    size_t file_size = file_stat.st_size;
    char *map_head = mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0); 
    if(map_head == MAP_FAILED) {
        ret_flag = ERR_MAP_FAILED;
        goto clean_and_return;
    }
#else
    HANDLE h_file = CreateFile(data_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(h_file == INVALID_HANDLE_VALUE) {
        ret_flag = ERR_FILE_OPEN;
        goto clean_and_return;
    }
    LARGE_INTEGER file_size_x;
    if(GetFileSizeEx(h_file, &file_size_x)==0) {
        ret_flag = ERR_FILE_STAT;
        goto clean_and_return;
    }
    size_t file_size = file_size_x.QuadPart;
    HANDLE h_file_mapping = CreateFileMapping(h_file, NULL, PAGE_READONLY, 0, 0, NULL);
    if(h_file_mapping == INVALID_HANDLE_VALUE) {
        ret_flag = ERR_MAP_FAILED;
        goto clean_and_return;
    }
    LPVOID lp_base = MapViewOfFile(h_file_mapping, FILE_MAP_READ | FILE_MAP_COPY, 0, 0, file_size);
    if(!lp_base) {
        ret_flag = ERR_MAP_FAILED;
        goto clean_and_return;
    }
    char *map_head = (char *)lp_base;
#endif
    char *p_tmp = map_head;
    size_t idx_tmp = 0, matched_counter = 0;
    for(size_t pos = 0; pos < file_size; ++pos) {
        if(map_head[pos] == '\n') {
            //map_head[pos] = '\0'; /* Now we use the kmp_search_ultra, no flip needed. */
            size_t line_len = pos - idx_tmp;
            if(kmp_search_ultra(p_tmp, line_len, search_kwd, kwd_len, next_array) >= 0) {
                if(insert_matched_line(matched_list, p_tmp, line_len) != 0) {
                    ret_flag = ERR_LIST_INSERT;
                    break;
                }
                ++matched_counter;
            }
            p_tmp += (line_len + 1);
            idx_tmp = pos + 1;
        }
    }
#ifdef _WIN32
    if(!UnmapViewOfFile(lp_base) || !CloseHandle(h_file_mapping))
        ret_flag = ERR_UNMAP_FAILED;
#else
    if(munmap(map_head, file_size) == -1) 
        ret_flag = ERR_UNMAP_FAILED;
#endif
        
clean_and_return:
    if(ret_flag != ERR_FILE_OPEN)
#ifdef _WIN32
        CloseHandle(h_file);
#else
        close(fd);
#endif
    if(ret_flag != ERR_FILE_OPEN && ret_flag != ERR_NULL_PTR && ret_flag != ERR_MEM_ALLOC)
        free(next_array);
    (ret_flag && ret_flag != ERR_UNMAP_FAILED) ? (*matched_line_num = 0) : (*matched_line_num = matched_counter);
    return ret_flag;
}

/**
 * Parse the CSV file and find the lines with the "search_kwd". 
 */
int csv_parser_arr(const char *data_file, char *search_kwd, struct matched_array **matched_array, size_t *matched_line_num) {
    int ret_flag = 0;
#ifndef _WIN32
    int fd = open(data_file, O_RDONLY);
    if(fd == -1) {
        ret_flag = ERR_FILE_OPEN;
        goto clean_and_return;
    }
#endif
    if(!search_kwd || !matched_array || !matched_line_num) {
        ret_flag = ERR_NULL_PTR;
        goto clean_and_return;
    }
    size_t kwd_len = strlen(search_kwd);
    size_t *next_array = kmp_create_next_array_new(search_kwd, kwd_len);
    if(!next_array) {
        ret_flag = ERR_MEM_ALLOC;
        goto clean_and_return;
    }
    *matched_array = (struct matched_array *)malloc(sizeof(struct matched_array) * LIST_SIZE_STEP);
    if(!(*matched_array)) {
        free(next_array);
        ret_flag = ERR_MEM_ALLOC;
        goto clean_and_return;
    }
 #ifndef _WIN32
    struct stat file_stat;
    if(fstat(fd, &file_stat) == -1) {
        ret_flag = ERR_FILE_STAT;
        goto clean_and_return;
    }
    size_t file_size = file_stat.st_size;
    char *map_head = mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0); 
    if(map_head == MAP_FAILED) {
        ret_flag = ERR_MAP_FAILED;
        goto clean_and_return;
    }
#else
    HANDLE h_file = CreateFile(data_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(h_file == INVALID_HANDLE_VALUE) {
        ret_flag = ERR_FILE_OPEN;
        goto clean_and_return;
    }
    LARGE_INTEGER file_size_x;
    if(GetFileSizeEx(h_file, &file_size_x)==0) {
        ret_flag = ERR_FILE_STAT;
        goto clean_and_return;
    }
    size_t file_size = file_size_x.QuadPart;
    HANDLE h_file_mapping = CreateFileMapping(h_file, NULL, PAGE_READONLY, 0, 0, NULL);
    if(h_file_mapping == INVALID_HANDLE_VALUE) {
        ret_flag = ERR_MAP_FAILED;
        goto clean_and_return;
    }
    LPVOID lp_base = MapViewOfFile(h_file_mapping, FILE_MAP_READ | FILE_MAP_COPY, 0, 0, file_size);
    if(!lp_base) {
        ret_flag = ERR_MAP_FAILED;
        goto clean_and_return;
    }
    char *map_head = (char *)lp_base;
#endif
    char *p_tmp = map_head;
    size_t idx_tmp = 0, matched_counter = 0, block_counter = 1;
    for(size_t pos = 0; pos < file_size; ++pos) {
        if(map_head[pos] == '\n') {
            //map_head[pos] = '\0'; /* Now we use the kmp_search_ultra, no flip needed. */
            size_t line_len = pos - idx_tmp;
            if(kmp_search_ultra(p_tmp, line_len, search_kwd, kwd_len, next_array) >= 0) {
                if(matched_counter >= (block_counter * LIST_SIZE_STEP)) {
                    ++block_counter;
                    struct matched_array *tmp = (struct matched_array *)realloc(*matched_array, \
                                                 block_counter * LIST_SIZE_STEP * \
                                                 sizeof(struct matched_array));
                    if(!tmp) {
                        ret_flag = ERR_ARR_GROW;
                        free(*matched_array); /* No allocation inside the array. */
                        goto memory_unmap;
                    }
                    *matched_array = tmp;
                }
                (*matched_array)[matched_counter].src_addr = p_tmp;
                (*matched_array)[matched_counter].line_len = line_len;
                ++matched_counter;
            }
            p_tmp += (line_len + 1);
            idx_tmp = pos + 1;
        }
    }
    for(size_t i = 0; i < matched_counter; ++i) {
        if(!((*matched_array)[i].matched_line = (char *)malloc(((*matched_array)[i].line_len + 1) * sizeof(char)))) {
            ret_flag = ERR_ARR_COPY; /* If this err occured, *matched_array will be returned. */
            for(size_t j = 0; j < i; ++j)
                free((*matched_array)[j].matched_line);
            break;
        }
        memcpy((*matched_array)[i].matched_line, (*matched_array)[i].src_addr, (*matched_array)[i].line_len);
        (*matched_array)[i].matched_line[(*matched_array)[i].line_len] = '\0';
    }
memory_unmap:
#ifdef _WIN32
    if(!UnmapViewOfFile(lp_base) || !CloseHandle(h_file_mapping))
        ret_flag = ERR_UNMAP_FAILED;
#else
    if(munmap(map_head, file_size) == -1) 
        ret_flag = ERR_UNMAP_FAILED;
#endif
        
clean_and_return:
    if(ret_flag != ERR_FILE_OPEN)
#ifdef _WIN32
        CloseHandle(h_file);
#else
        close(fd);
#endif
    if(ret_flag != ERR_FILE_OPEN && ret_flag != ERR_NULL_PTR && ret_flag != ERR_MEM_ALLOC)
        free(next_array);
    (ret_flag && ret_flag != ERR_UNMAP_FAILED) ? (*matched_line_num = 0) : (*matched_line_num = matched_counter);
    return ret_flag;
}


int main(int argc, char **argv) {
    size_t matched_line_num;
    clock_t start, end;
    int ret;
    if(argc == 1) {
        printf("Please specify \"arr\" or \"list\" or \"all\"\n");
        return 1;
    }
    if(strcmp(argv[1], "arr") == 0 || strcmp(argv[1], "all") == 0) {
        long total_elapsed = 0;
        for(size_t i = 0; i < TEST_ROUNDS; i++) {
            struct matched_array *matched_array = NULL;
            start = clock();
            ret = csv_parser_arr("./data/Table_1_Authors_career_2023_pubs_since_1788_wopp_extracted_202408_justnames.csv", ",Harvard", &matched_array, &matched_line_num);
            end = clock();
            total_elapsed += (end - start);
            for(size_t j = 0; j < matched_line_num; j++) {
                if(i == (TEST_ROUNDS - 1) && j >= (matched_line_num - 3)) 
                    printf("%s\n", matched_array[j].matched_line);
                free(matched_array[j].matched_line);
            }
            free(matched_array);
            printf("arr:\tround:\t%lu\tmatched lines:\t%lu\ttime_elapsed:\t%lf ms\n", i + 1, matched_line_num, (double)(end - start) * 1000 / CLOCKS_PER_SEC);
        }
        printf("\narr\ttime_elapsed_avg:\t%lf ms\n", (double)(total_elapsed) * 1000 / CLOCKS_PER_SEC / TEST_ROUNDS);
    }
    if(strcmp(argv[1], "list") == 0 || strcmp(argv[1], "all") == 0) {
        long total_elapsed = 0;
        for(size_t i = 0; i < TEST_ROUNDS; i++) {
            struct slist *matched_list = NULL;
            start = clock();
            ret = csv_parser("./data/Table_1_Authors_career_2023_pubs_since_1788_wopp_extracted_202408_justnames.csv", ",Harvard", &matched_list, &matched_line_num);
            end = clock();
            total_elapsed += (end - start);
            struct slist *tmp = matched_list, *tmp_next;
            for(size_t j = 0; j < matched_line_num; j++) {
                if(i == (TEST_ROUNDS - 1) && j >= (matched_line_num - 3)) 
                    printf("%s\n", tmp->matched_line);
                tmp_next = tmp->next;
                free(tmp);
                tmp = tmp_next;
            }
            printf("list:\tround:\t%lu\tmatched lines:\t%lu\ttime_elapsed:\t%lf ms\n", i + 1, matched_line_num, (double)(end - start) * 1000 / CLOCKS_PER_SEC);
        }
        printf("\nlist\ttime_elapsed_avg:\t%lf ms\n", (double)(total_elapsed) * 1000 / CLOCKS_PER_SEC / TEST_ROUNDS);
    }
    return 0;
}
