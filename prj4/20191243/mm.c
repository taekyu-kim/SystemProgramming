/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "mm.h"
#include "memlib.h"


/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your information in the following struct.
 ********************************************************/
team_t team = {
    /* Your student ID */
    "20191243",
    /* Your full name*/
    "Taekyu Kim",
    /* Your email address */
    "taekyu.s.kim@gmail.com",
};

/*
 * Constants
 */

#define ALIGNMENT   8               /* double word (8) alignment */
#define NUM_LISTS   32              /* total number of segregated lists */
#define MALLOCBUF   1 << 12         /* minimum size of allocation */
#define REALLOCBUF  1 << 8          /* minimum additional size during reallocation */
#define HEADER_SIZE 4               /* sizeof(int): size of block header */

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/*
 * Alignment Functions
 */
#define ALIGN(size)           (((size) + (ALIGNMENT-1)) & ~0x7)                 /* align a size */
#define ALIGN_FOR_BLOCK(size) (((size) < ALIGNMENT) ? (ALIGNMENT << 1) : \
                              (((size) + HEADER_SIZE + ALIGNMENT - 1) & ~0x7))  /* align a size with header */

/*
 * Block Header-Related Functions
 */

#define BLOCK_SIZE(ptr)             (*((unsigned int*)(ptr)) & ~0x7)            /* Retrieves the size of the block (header + payload) */
#define PAYLOAD_SIZE(ptr)           (BLOCK_SIZE(ptr) - HEADER_SIZE)             /* Retrieves the size of the payload (block size - header size) */
#define PAYLOAD_ADDR(ptr)           ((ptr) + HEADER_SIZE)                       /* Gets the payload address from the block address */
#define HEADER_ADDR(ptr)            ((char*)(ptr) - HEADER_SIZE)                /* Gets the block address from the payload address */
#define SET_HEADER(ptr, size)       (*((unsigned int*)(ptr)) = (size) | 1)      /* Sets and marks the block header as allocated */
#define FREE_HEADER(ptr, size)      do { \                                  
                                       *((unsigned int*)(ptr)) = (size); \
                                       *((unsigned int*)((ptr) + (size) - HEADER_SIZE)) = (size); \
                                   } while(0)                                   /* Sets the block header and footer as free */
#define NEXT_BLOCK(ptr)             ((ptr) + (*(unsigned int*)(ptr) & ~0x7))    /* Gets the next block in the heap */
#define NEXT_LIST_HEADER(ptr)       ((ptr) + 2 * HEADER_SIZE)                   /* Gets the next segregated list head */
#define IS_SET(ptr)                 ((*((unsigned int*)(ptr)) & 1) != 0)        /* Checks if a block is marked as allocated */


/*
 * Free List Related Functions
 */
#define NEXT_FREE_BLOCK_VAL(ptr)          (*(int*)((ptr) + HEADER_SIZE))                /* Gets the value of the next free block */
#define PREV_FREE_BLOCK_VAL(ptr)          (*(int*)((ptr) + 2 * HEADER_SIZE))            /* Gets the value of the previous free block */
#define SET_NEXT_FREE_BLOCK(ptr, nextptr) (*(int*)((ptr) + HEADER_SIZE) = (int)((nextptr) - (ptr)))
                                                                                        /* Sets the next free block pointer */
#define SET_PREV_FREE_BLOCK(ptr, prevptr) (*(int*)((ptr) + 2 * HEADER_SIZE) = (int)((prevptr) - (ptr)))
                                                                                        /* Sets the previous free block pointer */
#define NEXT_FREE_BLOCK(ptr)              ((ptr) + *(int*)((ptr) + HEADER_SIZE))        /*Gets the pointer to the next free block*/
#define PREV_FREE_BLOCK(ptr)              ((ptr) + *(int*)((ptr) + 2 * HEADER_SIZE))    /* Gets the pointer to the previous free block */
#define IS_END(ptr)                       (*(int*)((ptr) + HEADER_SIZE) == 0)           /* Checks if reached the end of the free list */


/*
 * Heap Space Related Functions
 */ 
#define IS_WITHIN_HEAP(ptr)        ((ptr) <= mem_heap_hi())             /* Checks if a pointer does not exceed the heap */
#define GET_LAST_BLOCK_IF_FREE()   (prev_block(mem_heap_hi() + 1))      /* Gets the last block in the heap, only if it is free */




/********************
 * Global Variables *
 ********************/

static char *head;  /* Points to the start of segregated free list heads */
static char *first; /* Points to the first allocated/freed block */



/********************
 * Function Headers *
 ********************/

int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);
static void split_block(char *block_ptr, int newsize);
static char *prev_block(char *block_ptr);
static char *search_list(int size);
static void insert_list(char *block_ptr);
static void remove_block(char *block_ptr);


/*******************************
 * Memory Management Functions *
 *******************************/

/* 
 * mm_init - Initialize the malloc package.
 *     Sets up segregated free list heads and extends heap by initial heap size.
 */
int mm_init(void) {
    // init_size - 모든 세분화된 리스트의 헤더를 포함할 수 있는 크기, 정렬된 상태로 계산
    int init_size = ((2 * HEADER_SIZE * NUM_LISTS + HEADER_SIZE + ALIGNMENT - 1) & ~0x7) - HEADER_SIZE;
    
    // 메모리를 할당하고 실패 시 -1을 반환
    head = (char*)mem_sbrk(init_size);
    if (head == (char*)-1) {
        return -1;
    }
    
    // 세분화된 리스트 초기화
    memset(head, 0, mem_heapsize());
    first = (char*)mem_heap_hi() + 1;

    // 초기 힙 할당 및 초기화
    void* initial_alloc = mm_malloc(MALLOCBUF);
    if (initial_alloc != NULL) {
        mm_free(initial_alloc);
    }
    else return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
size_t round_up_to_power_of_two(size_t x) {
    if (x <= 1) return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    // 추가로 32비트 이상을 다루는 경우를 위한 라인
    if (sizeof(size_t) > 4) x |= x >> 32;
    return x + 1;
}

// 적합한 자유 블록이 없을 때, 새로운 블록을 할당하고 초기화하는 함수
void *allocate_new_block(int32_t required_size) {
    int32_t buffer_size = MAX(required_size, MALLOCBUF); // 필요한 크기와 MALLOCBUF 중 더 큰 값 사용
    char *block_ptr = (char *)mem_sbrk(buffer_size);     // 메모리를 할당 요청

    if (block_ptr == (char *)-1) {   // 메모리 할당이 실패했을 경우
        return NULL;                 // NULL 반환
    }

    // 새로 할당된 블록의 헤더 설정
    FREE_HEADER(block_ptr, buffer_size);
    // 필요한 경우 블록 분할
    split_block(block_ptr, required_size);
    // 사용자에게 반환할 블록의 주소 반환
    return (void *)PAYLOAD_ADDR(block_ptr);
}

void *mm_malloc(size_t size) {
    if (!size) return NULL;

    /* if less than MALLOCBUF, round to nearest 2's power */
    if (size < MALLOCBUF) {
       size = round_up_to_power_of_two(size);
    }

    char *list, *block_ptr;
    int newsize = ALIGN_FOR_BLOCK(size), bufsize;

    /* search segregated free list for available free blocks, starting at appropriate list, in ascending order */
    list = search_list(newsize);
    while (list <= first - ALIGNMENT) {
        block_ptr = NEXT_FREE_BLOCK(list);
        while (block_ptr) { // block_ptr이 NULL이 아닐 때까지, 또는 IS_END로 끝을 확인할 때까지
            if (BLOCK_SIZE(block_ptr) >= newsize) {  // 적합한 크기의 블록 찾기
                remove_block(block_ptr);            // 자유 리스트에서 해당 블록 제거
                split_block(block_ptr, newsize);        // 필요하다면 블록 분할
                return (void *)PAYLOAD_ADDR(block_ptr); // 할당된 블록의 사용자 데이터 주소 반환
            }
            if (IS_END(block_ptr)) {          // 블록이 리스트의 끝을 나타내면
                break; // 내부 루프 탈출
            }
            block_ptr = NEXT_FREE_BLOCK(block_ptr); // 다음 블록으로 이동
        }
        list = NEXT_LIST_HEADER(list); // 다음 크기 범주의 리스트로 이동
    }
    /* there is no suitable free block; assign a new size that is at least MALLOCBUF */
    bufsize = newsize > MALLOCBUF ? newsize : MALLOCBUF;
    if ((block_ptr = allocate_new_block(newsize)) == NULL)
        return NULL;
    return block_ptr;
}


/*
 * mm_free - Freeing a block does nothing.
 */
void coalesce_free_block(char *block_ptr, int size) {
    FREE_HEADER(block_ptr, size);  // 현재 블록의 헤더를 설정하여 자유 상태로 만듬
    insert_list(block_ptr);  // 변경된 블록 크기로 세분화된 리스트에 삽입
}

void mm_free(void *ptr) {
    if (!ptr) return;

    char *block_ptr = HEADER_ADDR(ptr);
    int current_size = BLOCK_SIZE(block_ptr);

    // 이전 블록과 다음 블록을 확인하여 병합 가능한지 판단
    char *prev_block_ptr = prev_block(block_ptr); // 이전 블록의 헤더 주소
    char *next_block_ptr = NEXT_BLOCK(block_ptr); // 현재 블록 다음의 블록 주소

    // 이전 블록이 free인 경우 병합
    if (prev_block_ptr && !IS_SET(prev_block_ptr)) {
        remove_block(prev_block_ptr);
        current_size += BLOCK_SIZE(prev_block_ptr);
        block_ptr = prev_block_ptr;
    }

    // 다음 블록이 free이고 heap 범위 내에 있는 경우 병합
    bool can_coalesce_next = IS_WITHIN_HEAP(next_block_ptr) && !IS_SET(next_block_ptr);
    if (can_coalesce_next) {
        remove_block(next_block_ptr);
        current_size += BLOCK_SIZE(next_block_ptr);
    }

    // 현재 블록을 자유 블록으로 설정하고 segregated list에 삽입
    coalesce_free_block(block_ptr, current_size);

    return;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    if (ptr == NULL) return mm_malloc(size);
    if (!size) {
        mm_free(ptr);
        return NULL;
    }

    /* if less than REALLOCBUF, round to nearest 2's power */
    if (size < REALLOCBUF) {
        size = round_up_to_power_of_two(size);
    }

    char *blkptr = HEADER_ADDR(ptr);
    char *nextblkptr = NEXT_BLOCK(blkptr), *prevblkptr = prev_block(blkptr);
    int oldsize = BLOCK_SIZE(blkptr), newsize = ALIGN_FOR_BLOCK(size), combsize;

    // Check if the existing block size is sufficient.
    if (newsize <= PAYLOAD_SIZE(blkptr))
        return ptr;

    // Coalesce with adjacent free blocks if possible and needed.
    if (prevblkptr && !IS_SET(prevblkptr) && BLOCK_SIZE(prevblkptr) > REALLOCBUF) {
        combsize = BLOCK_SIZE(prevblkptr) + oldsize;
        if (IS_WITHIN_HEAP(nextblkptr) && !IS_SET(nextblkptr)) {
            combsize += BLOCK_SIZE(nextblkptr);
            remove_block(nextblkptr);
        }
        if (combsize >= newsize) {
            remove_block(prevblkptr);
            memmove(prevblkptr, blkptr, oldsize);
            blkptr = prevblkptr;
            SET_HEADER(blkptr, combsize);
            return (void *)PAYLOAD_ADDR(blkptr);
        }
    }
    else if (IS_WITHIN_HEAP(nextblkptr) && !IS_SET(nextblkptr)) {
        combsize = BLOCK_SIZE(nextblkptr) + oldsize;
        if (combsize >= newsize) {
            remove_block(nextblkptr);
            SET_HEADER(blkptr, combsize);
            return (void *)PAYLOAD_ADDR(blkptr);
        }
    }

    // If extending is needed, attempt to extend the heap.
    if (!IS_WITHIN_HEAP(nextblkptr)) {
        int bufsize = ALIGN_FOR_BLOCK(newsize - oldsize);
        if (mem_sbrk(bufsize) == (void *)-1)
            return NULL;
        SET_HEADER(blkptr, oldsize + bufsize);
        return (void *)PAYLOAD_ADDR(blkptr);
    }

    // Allocate a new block if no coalescing or extension is possible.
    char *newblkptr = (char*)mm_malloc(newsize);
    if (!newblkptr)
        return NULL;
    memcpy(newblkptr, ptr, PAYLOAD_SIZE(blkptr));
    mm_free(ptr);
    return newblkptr;
}



/********************
 * Helper Functions *
 ********************/

 /*
  * split_block - Allocate a free block with new size, and split if abundant
  */
static void split_block(char *blkptr, int newsize) {
    char *newblkptr;
    int oldsize = BLOCK_SIZE(blkptr);
    if (oldsize <= newsize + ALIGNMENT) { /* there is enough space to split the block */
        SET_HEADER(blkptr, oldsize);
    }
    else {                               /* this block cannot be split */
        SET_HEADER(blkptr, newsize);
        newblkptr = NEXT_BLOCK(blkptr);
        FREE_HEADER(newblkptr, oldsize - newsize); /* set header for split block */
        insert_list(newblkptr);     /* insert split block into segregated list */
    }
}


/*
 * prev_block - Return the previous block if it is free; NULL otherwise.
 */
static bool is_prev_block_valid(char *header, char *footer) {
    return !(IS_SET(footer) ||
             header < first ||
             header > footer - HEADER_SIZE * 3 ||
             (uintptr_t)(header + HEADER_SIZE) % ALIGNMENT != 0 ||
             BLOCK_SIZE(header) != BLOCK_SIZE(footer) ||
             IS_SET(header));
}

// Helper function to check if the block is in the segregated free list
static bool is_block_in_free_list(char *header) {
    char *list = search_list(header);
    char* tempblkptr;
    for (tempblkptr = NEXT_FREE_BLOCK(list);; tempblkptr = NEXT_FREE_BLOCK(tempblkptr)) {
        if (header == tempblkptr)
            return true;
        else if (IS_END(tempblkptr))
            break; 
    }
    return false;
}

static char *prev_block(char *blkptr) {
    char *prevblk_footer = blkptr - HEADER_SIZE;
    char *prevblk_header = blkptr - BLOCK_SIZE(prevblk_footer);

    if (!is_prev_block_valid(prevblk_header, prevblk_footer)) {
        return NULL;
    }

    /* finally, check if the block is officially a free block by looking into segregated list */
    if (is_block_in_free_list(prevblk_header)) {
        return prevblk_header;
    }
    else return NULL;
}

/*
 * search_free_list - Returns head to a free list among segregated lists that best-fits given size
 */
static char *search_list(int size) {
    char *list = head;
    int threshold = 16;  // 초기 threshold 값 설정
    int list_num = 1;    // 리스트 번호 초기화

    // 주어진 크기가 threshold를 초과하지 않고, 리스트의 최대 수에 도달하지 않을 때까지 반복
    while (size > threshold && list_num < NUM_LISTS) {
        threshold <<= 1;  // threshold 값을 배로 증가
        list_num++;
        list = NEXT_LIST_HEADER(list);  // 다음 리스트 헤더로 이동
    }
    return list;
}


/*
 * insert_into_list - Inserts a new free block into free list, in ascending order.
 */
static void link_blocks(char *prev_block, char *current_block) {
    SET_NEXT_FREE_BLOCK(prev_block, current_block);
    SET_PREV_FREE_BLOCK(current_block, prev_block);
}

static void insert_list(char *blkptr)
{
    /* first, find the list that best-fits block's size */
    char *list = search_list(BLOCK_SIZE(blkptr));

    if (IS_END(list)) {
        SET_NEXT_FREE_BLOCK(blkptr, blkptr);
        link_blocks(list, blkptr);
        return;
    }
    else {
        while (1) {
            /* insert when the block not greater than next block, or */
            /* there is no more block in the list                    */
            if (BLOCK_SIZE(NEXT_FREE_BLOCK(list)) >= BLOCK_SIZE(blkptr)) {
                /* found the appropriate position */
                link_blocks(blkptr, NEXT_FREE_BLOCK(list));
                link_blocks(list, blkptr);
                return;
            }
            else if (IS_END(NEXT_FREE_BLOCK(list))) {
                /* there is no more block to compare to */
                SET_NEXT_FREE_BLOCK(blkptr, blkptr);
                link_blocks(NEXT_FREE_BLOCK(list), blkptr);
                return;
            }
            /* none of above condition holds; go to next item in the list */
            list = NEXT_FREE_BLOCK(list);
        }
    }
}

/*
 * remove_from_list - Removes a free block from free list.
 */
static void remove_block(char *block_ptr)
{
    char *prevblkptr = PREV_FREE_BLOCK(block_ptr);
    char *nextblkptr = NEXT_FREE_BLOCK(block_ptr);
    if (IS_END(block_ptr)) /* if at the end of free list, just fix the previous block */
        SET_NEXT_FREE_BLOCK(prevblkptr, prevblkptr);
    else { /* if in the middle of free list, connect previous and next free blocks */
        link_blocks(prevblkptr, nextblkptr);
    }
    return;
}