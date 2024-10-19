#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

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
    if (string == NULL || (length = strlen(string)) < 1) 
        return NULL;
    size_t* next_array = (size_t*)calloc(length, sizeof(size_t));
    if (next_array == NULL) 
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

/* The standard KMP search algorithm. It frees the NEXT array. */
int64_t kmp_search_std(char line[], char search_substr[]) {
    if (line == NULL || search_substr == NULL) 
        return -1;
    size_t line_len = strlen(line), key_len = strlen(search_substr), i = 0, j = 0;
    if (line_len < key_len || key_len == 0) 
        return -1;
    size_t* next_array = kmp_create_next_array(search_substr);
    if (next_array == NULL)
        return -1;
    while (i < line_len) {
        if (line[i] == search_substr[j]) {
            i++; j++;
            if (j == key_len) {
                free(next_array);
                return i - j;
            }
        }
        else {
            if (j) { j = next_array[j - 1]; }
            else   { i++; }
        }
    }
    free(next_array); // NEXT array freed.
    return -1;
}

/* Fast version: the NEXT array is managed outside of this function, 
 * The NEXT array MUST be generated by the search_substr ! */
int64_t kmp_search_fast(char line[], char search_substr[], size_t next_array[]) {
    if (line == NULL || search_substr == NULL) 
        return -1;
    size_t line_len = strlen(line), key_len = strlen(search_substr), i = 0, j = 0;
    if (line_len < key_len || key_len == 0) 
        return -1;
    if (next_array == NULL)
        return -1;
    while (i < line_len) {
        if (line[i] == search_substr[j]) {
            i++; j++;
            if (j == key_len)
                return i - j;
        }
        else {
            if (j) { j = next_array[j - 1]; }
            else   { i++; }
        }
    }
    return -1;
}

/* Ultra version: the NEXT array is managed outside of this function,
 * Put line_len and substr_len as arguments because they are known for the call 
 * The NEXT array MUST be generated by the search_substr ! */
int64_t kmp_search_ultra(char line[], size_t line_len, char search_substr[], size_t substr_len, size_t next_array[]) {
    if (line == NULL || search_substr == NULL) 
        return -1;
    size_t i = 0, j = 0;
    if (line_len < substr_len || substr_len == 0) 
        return -1;
    if (next_array == NULL)
        return -1;
    while (i < line_len) {
        if (line[i] == search_substr[j]) {
            i++; j++;
            if (j == substr_len)
                return i - j;
        }
        else {
            if (j) { j = next_array[j - 1]; }
            else   { i++; }
        }
    }
    return -1;
}

#define TEST_ROUNDS         50
#define MATCHED_LIST_MAX    65536

#define ERR_FILE_OPEN       -1
#define ERR_NULL_PTR        -3
#define ERR_MEM_ALLOC       -5
#define ERR_FILE_STAT       -7
#define ERR_MAP_FAILED      -9
#define ERR_ALLOC_IN_LOOP   -11
#define ERR_LIST_BOUND      -13
#define ERR_UNMAP_FAILED    -15

/**
 * Parse the CSV file and find the lines with the "search_kwd".
 * The matched_list should be dynamic, but for demo purpose we didn't do that 
 */
int csv_parser(const char *data_file, char *search_kwd, char **matched_list, const size_t matched_list_max) {
    int ret_flag = 0;
#ifndef _WIN32
    int fd = open(data_file, O_RDONLY);
    if(fd == -1) {
        ret_flag = ERR_FILE_OPEN;
        goto clean_and_return;
    }
#endif
    if(search_kwd == NULL || matched_list == NULL) {
        ret_flag = ERR_NULL_PTR;
        goto clean_and_return;
    }
    size_t *next_array = kmp_create_next_array(search_kwd);
    if(next_array == NULL) {
        ret_flag = ERR_MEM_ALLOC;
        goto clean_and_return;
    }
    size_t matched_line_num = 0, kwd_len = strlen(search_kwd);
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
    //printf("%lu\n", file_size);
    HANDLE h_file_mapping = CreateFileMapping(h_file, NULL, PAGE_READONLY, 0, 0, NULL);
    if(h_file_mapping == INVALID_HANDLE_VALUE) {
        ret_flag = ERR_MAP_FAILED;
        goto clean_and_return;
    }
    LPVOID lp_base = MapViewOfFile(h_file_mapping, FILE_MAP_READ | FILE_MAP_COPY, 0, 0, file_size);
    if(lp_base == NULL) {
        ret_flag = ERR_MAP_FAILED;
        goto clean_and_return;
    }
    //printf("%lu\n", file_size);
    char *map_head = (char *)lp_base;
#endif
    char *p_tmp = map_head;
    size_t idx_tmp = 0, line_len;
    for(size_t pos = 0; pos < file_size; pos++) {
        if(map_head[pos] == '\n') {
            //map_head[pos] = '\0'; /* Now we use the kmp_search_ultra, no flip needed. */
            line_len = pos - idx_tmp;
            if(kmp_search_ultra(p_tmp, line_len, search_kwd, kwd_len, next_array) >= 0) {
                char *line_tmp = (char *)malloc((line_len + 1) * sizeof(char));
                if(line_tmp == NULL) {
                    ret_flag = ERR_ALLOC_IN_LOOP;
                    break;
                }
                memcpy(line_tmp, p_tmp, line_len); 
                line_tmp[line_len] = '\0';
                matched_list[matched_line_num] = line_tmp;
                matched_line_num++;
                if(matched_line_num == matched_list_max) {
                    ret_flag = ERR_LIST_BOUND;
                    break;
                }
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
    return (ret_flag) ? ret_flag : matched_line_num;
}

int main(int argc, char **argv) {
    char *matched_lines[MATCHED_LIST_MAX] = {NULL,};
    clock_t start = clock();
    int ret;
    for(size_t i = 0; i < TEST_ROUNDS; i++) {
        ret = csv_parser("./data/Table_1_Authors_career_2023_pubs_since_1788_wopp_extracted_202408_justnames.csv", ",Harvard", matched_lines, MATCHED_LIST_MAX);
    }
    clock_t end = clock();
    for(size_t i = 0; i < ret; i++) {
        printf("%s\n", matched_lines[i]);
        free(matched_lines[i]);
    }
    printf("\nmatched lines:\t%d ---------\n", ret);
    printf("time_elapsed:\t%lf ms\n", (double)(end - start) * 1000 / CLOCKS_PER_SEC / TEST_ROUNDS);
    return 0;
}
