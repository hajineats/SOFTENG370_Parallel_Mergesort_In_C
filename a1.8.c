/*
    The Hybrid Merge Sort to use for Operating Systems Assignment 1 2021
    written by Robert Sheehan

    Modified by: Hajin Kim
    UPI: hkim532

    By submitting a program you are claiming that you and only you have made
    adjustments and additions to this code.
 */

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h>
#include <sys/resource.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/times.h>
#include <math.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/mman.h>


#define SIZE    4
#define MAX     1000
#define SPLIT   16

struct block {
    int size;
    int *data;
};

void print_data(struct block *block) {
    for (int i = 0; i < block->size; ++i)
        printf("%d ", block->data[i]);
    printf("\n");
}

/* Check to see if the data is sorted. */
bool is_sorted(struct block *block) {
    bool sorted = true;
    for (int i = 0; i < block->size - 1; i++) {
        if (block->data[i] > block->data[i + 1])
            sorted = false;
    }
    return sorted;
}

/* Fill the array with random data. */
void produce_random_data(struct block *block) {
    srand(1); // the same random data seed every time
    for (int i = 0; i < block->size; i++) {
        block->data[i] = rand() % MAX;
    }
}
///////////
//
//////////

/* The insertion sort for smaller halves. */
void insertion_sort(struct block *block) {
    for (int i = 1; i < block->size; ++i) {
        for (int j = i; j > 0; --j) {
            if (block->data[j-1] > block->data[j]) {
                int temp;
                temp = block->data[j-1];
                block->data[j-1] = block->data[j];
                block->data[j] = temp;
            }
        }
    }
}

/* Combine the two halves back together. */
void merge(struct block *left, struct block *right) {
    int *combined = calloc(left->size + right->size, sizeof(int));
    if (combined == NULL) {
        perror("Allocating space for merge.\n");
        exit(EXIT_FAILURE);
    }
        int dest = 0, l = 0, r = 0;
        while (l < left->size && r < right->size) {
                if (left->data[l] < right->data[r])
                        combined[dest++] = left->data[l++];
                else
                        combined[dest++] = right->data[r++];
        }
        while (l < left->size)
                combined[dest++] = left->data[l++];
        while (r < right->size)
                combined[dest++] = right->data[r++];
    memmove(left->data, combined, (left->size + right->size) * sizeof(int));
    free(combined);
}

/* Merge sort the data. */
void *merge_sort(void *ptr) {
    struct block *block = (struct block *)ptr;
    if (block->size > SPLIT) {
        struct block left_block;
        struct block right_block;
        left_block.size = block->size / 2;
        left_block.data = block->data;
        right_block.size = block->size - left_block.size; // left_block.size + (block->size % 2);
        right_block.data = block->data + left_block.size;
        merge_sort(&left_block);
        merge_sort(&right_block);
        merge(&left_block, &right_block);
    } else {
        insertion_sort(block);
    }
    return NULL;
}

void merge_sort_init(struct block *incoming){
    // if(incoming->size <= SPLIT){
    //     insertion_sort(incoming);
    //     return;
    // }


    // copy the incoming block as mmap
    int *copied_data = mmap(NULL, incoming->size*sizeof(int), PROT_WRITE|PROT_READ, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    for(int i = 0; i<incoming->size; i++){
        copied_data[i] = incoming->data[i];
    }

    // for(int i = 0; i<incoming->size; i++){
    //     printf("%d ", copied_data[i]);
    // }

    // make a struct instance that will point to this mmap
    struct block block;
    block.data = copied_data;
    block.size = incoming->size;
    
    // make array of 8 blocks 
    struct block blocks[8];
    for(int i = 0; i<8; i++){
        blocks[i].data = block.data + (incoming->size/8)*i; // block.size/8 is how many elements will be in each chunk
        blocks[i].size = block.size/8;
    }

    pid_t *pid_array = mmap(NULL, 8*sizeof(pid_t), PROT_WRITE|PROT_READ, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pid_t n1 = fork();
    pid_t n2 = fork();
    pid_t n3 = fork();

    if (n1 == 0 && n2 == 0 && n3 == 0){
        pid_array[0] = getpid();
        merge_sort(&blocks[0]);
        exit(0);
    }else if(n1 == 0 && n2 == 0 && n3 != 0){
        pid_array[1] = getpid();        
        merge_sort(&blocks[1]);
        waitpid(pid_array[0], NULL, 0);
        merge(&blocks[0], &blocks[1]);

        exit(0);
    }else if(n1 == 0 && n2 != 0 && n3 == 0){
        pid_array[2] = getpid();
        merge_sort(&blocks[2]);
        exit(0);
    }else if(n1 == 0 && n2 != 0 && n3 != 0){
        pid_array[3] = getpid();
        merge_sort(&blocks[3]);
        waitpid(pid_array[2],NULL,0);
        merge(&blocks[2], &blocks[3]);
        
        waitpid(pid_array[1],NULL,0);
        blocks[0].size *= 2;
        blocks[2].size *= 2;
        merge(&blocks[0], &blocks[2]);
        
        exit(0);
    }else if(n1 != 0 && n2 == 0 && n3 == 0){
        pid_array[4] = getpid();
        merge_sort(&blocks[4]);
        exit(0);
    }else if(n1 != 0 && n2 == 0 && n3 != 0){
        pid_array[5] = getpid();
        merge_sort(&blocks[5]);
        waitpid(pid_array[4], NULL, 0);
        merge(&blocks[4], &blocks[5]);

        exit(0);
    }else if(n1 != 0 && n2 != 0 && n3 == 0){
        pid_array[6] = getpid();
        merge_sort(&blocks[6]);
        exit(0);
    }else{
        pid_array[7] = getpid();
        merge_sort(&blocks[7]);
        waitpid(pid_array[6], NULL, 0);
        merge(&blocks[6], &blocks[7]);
        blocks[6].size *= 2;


        waitpid(pid_array[5], NULL, 0);
        blocks[4].size *= 2;
        merge(&blocks[4], &blocks[6]);
        

        waitpid(pid_array[3], NULL, 0);
        blocks[0].size *= 4;
        blocks[4].size *= 2;
        merge(&blocks[0], &blocks[4]);
        blocks[0].size *= 2;

        for(int i = 0; i<incoming->size; i++){
            incoming->data[i] = blocks[0].data[i];
        }
    }
}

int main(int argc, char *argv[]) {
        long size;

        if (argc < 2) {
                size = SIZE;
        } else {
                size = atol(argv[1]);
        }
    struct block block;
    block.size = (int)pow(2, size);
    block.data = (int *)calloc(block.size, sizeof(int));
    if (block.data == NULL) {
        perror("Unable to allocate space for data.\n");
        exit(EXIT_FAILURE);
    }

    produce_random_data(&block);

    struct timeval start_wall_time, finish_wall_time, wall_time;
    struct tms start_times, finish_times;
    gettimeofday(&start_wall_time, NULL);
    times(&start_times);

// CUSTOM MERGESORT STARTS
    merge_sort_init(&block);
    // merge_sort(&block);
    
// CUSTOM MERGESORT ENDS
    gettimeofday(&finish_wall_time, NULL);
    times(&finish_times);
    timersub(&finish_wall_time, &start_wall_time, &wall_time);
    printf("start time in clock ticks: %ld\n", start_times.tms_utime);
    printf("finish time in clock ticks: %ld\n", finish_times.tms_utime);
    printf("wall time %ld secs and %d microseconds\n", wall_time.tv_sec, wall_time.tv_usec);

    if (block.size < 1025)
        print_data(&block);

    printf(is_sorted(&block) ? "sorted\n" : "not sorted\n");
    free(block.data);
    exit(EXIT_SUCCESS);
}