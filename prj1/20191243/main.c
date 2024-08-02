#include "mapping.h"
#include "list.h"
#include "hash.h"
#include "bitmap.h"
#include "limits.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT_LEN 48

int main(){
    char input[MAX_INPUT_LEN];
    const char delim[] = " ";
    int type; // type 1 - list , type 2 - hash, type 3 - bitmap

    int list_num = 0;
    int hash_num = 0;
    int bitmap_num = 0;

    while (1){
	
        char *tok[7] = { NULL };

        // input 받기
        fgets(input, MAX_INPUT_LEN, stdin);
        input[strlen(input) - 1] = '\0';

        // input을 token으로 쪼개기
        int i = 0;
        tok[i] = strtok(input, delim);
        if (strcmp(tok[0], "quit") == 0){ // command가 quit면 끝내기
            break;
        }
        while (tok[i] != NULL){
            tok[++i] = strtok(NULL, delim);
        }

        // input에 맞는 command 호출

        // create
        if (strcmp(tok[0], "create") == 0){
            if (strcmp(tok[1], "list") == 0){ // create list <LIST>
                type = 1;
                if (list_num == 10){
                    printf("CAN'T CREATE: MAXIMUM NUM OF LIST IS 10\n");
                }
                else{
                    if (create_list(tok[2]) != NULL){
                        list_num++;
                    }
                }
            }

            else if (strcmp(tok[1], "hashtable") == 0){ // create hashtable <HASH TABLE>
                type = 2; // hashtable는 type 2으로 설정
                if (hash_num == 10){
                    printf("CAN'T CREATE: MAXIMUM NUM OF HASHTABLE IS 10\n");
                }
                else{
                    if (create_hash(tok[2]) != NULL){
                        hash_num++;
                    }
                }
            }

            else if (strcmp(tok[1], "bitmap") == 0){ // create bitmap <BITMAP> <BIT CNT>
                type = 3; // bitmap은 type 3으로 설정
                if (bitmap_num == 10){
                    printf("CAN'T CREATE: MAXIMUM NUM OF BITMAP IS 10\n");
                }
                else{
                    if (create_bitmap(tok[2], tok[3]) != NULL){
                        bitmap_num++;
                    }
                }
            }
        }

        // delete
        else if (strcmp(tok[0], "delete") == 0){ // delete <LIST | HASH TABLE | BITMAP>
            switch (type){
                case 1:
                    if (delete_list(tok[1]) == true){
                        list_num--;
                    }
                    else{
                        printf("CAN'T DELETE: %s NOT CREATED\n", tok[1]);
                    }
                    break;

                case 2:
                    if (delete_hash(tok[1]) == true){
                        hash_num--;
                    }
                    else{
                        printf("CAN'T DELETE: %s NOT CREATED\n", tok[1]);
                    }
                    break;
                    
                case 3:
                    if (delete_bitmap(tok[1]) == true){
                        bitmap_num--;
                    }
                    else{
                        printf("CAN'T DELETE: %s NOT CREATED\n", tok[1]);
                    }                    
                    break;
            }
        }

        // dumpdata
        else if (strcmp(tok[0], "dumpdata") == 0){ // dumpdata <LIST | HASH TABLE | BITMAP>
            switch (type){
                case 1:
                    dumpdata_list(tok[1]);
                    break;  
                case 2:
                    dumpdata_hash(tok[1]);
                    break;
                case 3:
                    dumpdata_bitmap(tok[1]);
                    break;
            }
        }
        
        // list_...
        
        else if (strcmp(tok[0], "list_push_front") == 0){
            int idx = atoi(&tok[1][4]);
            struct list_item *new_item = (struct list_item *)malloc(sizeof(struct list_item));
            new_item->data = atoi(tok[2]);
            list_push_front(list_arr[idx], &new_item->elem);
        }
        
        else if (strcmp(tok[0], "list_push_back") == 0){
            int idx = atoi(&tok[1][4]);
            struct list_item *new_item = (struct list_item *)malloc(sizeof(struct list_item));
            new_item->data = atoi(tok[2]);
            list_push_back(list_arr[idx], &new_item->elem);
        }

        else if (strcmp(tok[0], "list_pop_front") == 0){
            int idx = atoi(&tok[1][4]);
            struct list *target_list = list_arr[idx];
            list_pop_front(target_list);
        }

        else if (strcmp(tok[0], "list_pop_back") == 0){
            int idx = atoi(&tok[1][4]);
            struct list *target_list = list_arr[idx];
            list_pop_back(target_list);
        }

        else if (strcmp(tok[0], "list_unique") == 0){
            int idx1 = atoi(&tok[1][4]);
            void *aux = NULL;
            if (tok[2] != NULL){
                int idx2 = atoi(&tok[2][4]);
                list_unique(list_arr[idx1], list_arr[idx2], list_less, aux);
            }
            else{
                struct list *aux_list = (struct list*)malloc(sizeof(struct list));
                list_init(aux_list);
                list_unique(list_arr[idx1], aux_list, list_less, aux);
                free(aux_list);
            }
        }

        else if (strcmp(tok[0], "list_swap") == 0){
            int idx = atoi(&tok[1][4]);
            int iter1 = atoi(tok[2]);
            int iter2 = atoi(tok[3]);
            if (iter1 > iter2){
                int n = iter1;
                iter1 = iter2;
                iter2 = n;
            }
            
            struct list_elem *temp = list_begin(list_arr[idx]);
            for (int i = 0; i < iter1; i++){
                temp = list_next(temp);
            }
            struct list_elem *target_elem1 = temp;

            temp = list_begin(list_arr[idx]);
            for (int i = 0; i < iter2; i++){
                temp = list_next(temp);
            }
            struct list_elem *target_elem2 = temp;

            list_swap(target_elem1, target_elem2);
        }

        else if (strcmp(tok[0], "list_splice") == 0){
            int dest_idx = atoi(&tok[1][4]);
            int insert_idx = atoi(tok[2]);

            int src_idx = atoi(&tok[3][4]);
            int start_idx = atoi(tok[4]);
            int end_idx = atoi(tok[5]);

            struct list_elem *before = list_begin(list_arr[dest_idx]);
            for (int i = 0; i < insert_idx; i++){
                before = list_next(before);
            }

            struct list_elem *first = list_begin(list_arr[src_idx]);
            for (int i = 0; i < start_idx; i++){
                first = list_next(first);
            }
            struct list_elem *last = list_begin(list_arr[src_idx]);
            for (int i = 0; i < end_idx; i++){
                last = list_next(last);
            }

            list_splice(before, first, last);
        }

        else if (strcmp(tok[0], "list_sort") == 0){
            int idx = atoi(&tok[1][4]);
            void *aux = NULL;
            list_sort(list_arr[idx], list_less, aux);
        }

        else if (strcmp(tok[0], "list_shuffle") == 0){
            int idx = atoi(&tok[1][4]);
            list_shuffle(list_arr[idx]);
        }

        else if (strcmp(tok[0], "list_reverse") == 0){
            int idx = atoi(&tok[1][4]);
            list_reverse(list_arr[idx]);
        }

        else if (strcmp(tok[0], "list_remove") == 0){
            int idx = atoi(&tok[1][4]);
            int iter = atoi(tok[2]);
            struct list_elem *target = list_begin(list_arr[idx]);
            for (int i = 0; i < iter; i++){
                target = list_next(target);
            }
            list_remove(target);
        }

        else if (strcmp(tok[0], "list_empty") == 0){
            int idx = atoi(&tok[1][4]);
            if (list_empty(list_arr[idx])){
                printf("true\n");
            }
            else{
                printf("false\n");
            }
        }

        else if (strcmp(tok[0], "list_size") == 0){
            int idx = atoi(&tok[1][4]);
            int cnt = list_size(list_arr[idx]);
            printf("%d\n", cnt);
        }

        else if (strcmp(tok[0], "list_max") == 0){
            int idx = atoi(&tok[1][4]);
            void *aux = NULL;
            struct list_elem *max = list_max(list_arr[idx], list_less, aux);
            printf("%d\n", list_entry(max, struct list_item, elem)->data);
        }

        else if (strcmp(tok[0], "list_min") == 0){
            int idx = atoi(&tok[1][4]);
            void *aux = NULL;
            struct list_elem *min = list_min(list_arr[idx], list_less, aux);
            printf("%d\n", list_entry(min, struct list_item, elem)->data);
        }

        else if (strcmp(tok[0], "list_insert") == 0){
            int idx = atoi(&tok[1][4]);
            int iter = atoi(tok[2]);
            struct list_elem *target = list_begin(list_arr[idx]);
            for (int i = 0; i < iter; i++){
                target = list_next(target);
            }
            struct list_item *new_item = (struct list_item *)malloc(sizeof(struct list_item));
            new_item->data = atoi(tok[3]);
            list_insert(target, &new_item->elem);
            list_arr[idx]->item_cnt++;
        }

        else if (strcmp(tok[0], "list_insert_ordered") == 0){
            int idx = atoi(&tok[1][4]);
            void *aux = NULL;
            struct list_item *new_item = (struct list_item *)malloc(sizeof(struct list_item));
            new_item->data = atoi(tok[2]);
            list_insert_ordered(list_arr[idx], &new_item->elem, list_less, aux);
            list_arr[idx]->item_cnt++;
        }

        else if (strcmp(tok[0], "list_front") == 0){
            int idx = atoi(&tok[1][4]);
            struct list_elem *front = list_front(list_arr[idx]);
            printf("%d\n", list_entry(front, struct list_item, elem)->data);
        }

        else if (strcmp(tok[0], "list_back") == 0){
            int idx = atoi(&tok[1][4]);
            struct list_elem *back = list_back(list_arr[idx]);
            printf("%d\n", list_entry(back, struct list_item, elem)->data);
        }

        // hash_ ...

        else if (strcmp(tok[0], "hash_insert") == 0){
            int idx = atoi(&tok[1][4]);
            struct hash_elem *new_elem = (struct hash_elem *)malloc(sizeof(struct hash_elem));
            new_elem->value = atoi(tok[2]);
            hash_insert(hash_arr[idx], new_elem);
        }

        else if (strcmp(tok[0], "hash_replace") == 0){
            int idx = atoi(&tok[1][4]);
            struct hash_elem *new_elem = (struct hash_elem *)malloc(sizeof(struct hash_elem));
            new_elem->value = atoi(tok[2]);
            hash_replace(hash_arr[idx], new_elem);
        }

        else if (strcmp(tok[0], "hash_find") == 0){
            int idx = atoi(&tok[1][4]);
            struct hash_elem *find_elem = (struct hash_elem *)malloc(sizeof(struct hash_elem));
            find_elem->value = atoi(tok[2]);
            struct hash_elem *target = hash_find(hash_arr[idx], find_elem);
            if (target != NULL){
                printf("%d\n", target->value);
            }
        }

        else if (strcmp(tok[0], "hash_empty") == 0){
            int idx = atoi(&tok[1][4]);
            if (hash_empty(hash_arr[idx])){
                printf("true\n");
            }
            else{
                printf("false\n");
            }
        }

        else if (strcmp(tok[0], "hash_size") == 0){
            int idx = atoi(&tok[1][4]);
            printf("%zu\n", hash_size(hash_arr[idx]));
        }

        else if (strcmp(tok[0], "hash_clear") == 0){
            int idx = atoi(&tok[1][4]);
            hash_clear(hash_arr[idx], destructor);
        }

        else if (strcmp(tok[0], "hash_delete") == 0){
            int idx = atoi(&tok[1][4]);
            struct hash_elem *temp = (struct hash_elem *)malloc(sizeof(struct hash_elem));
            temp->value = atoi(tok[2]);
            hash_delete(hash_arr[idx], temp);
        }

        else if (strcmp(tok[0], "hash_apply") == 0){
            int idx = atoi(&tok[1][4]);
            
            if (strcmp(tok[2], "square") == 0){
                hash_apply(hash_arr[idx], square);
            }
            else if (strcmp(tok[2], "triple") == 0){
                hash_apply(hash_arr[idx], triple);
            }
        }

        // bitmap_ ...

        else if (strcmp(tok[0], "bitmap_mark") == 0){
            int idx = atoi(&tok[1][2]);
            int bit_idx = atoi(tok[2]);
            bitmap_mark(bitmap_arr[idx], bit_idx);
        }

        else if (strcmp(tok[0], "bitmap_test") == 0){
            int idx = atoi(&tok[1][2]);
            int bit_idx = atoi(tok[2]);
            if (bitmap_test(bitmap_arr[idx], bit_idx)){
                printf("true\n");
            }
            else{
                printf("false\n");
            }
        }

        else if (strcmp(tok[0], "bitmap_size") == 0){
            int idx = atoi(&tok[1][2]);
            printf("%zu\n", bitmap_size(bitmap_arr[idx]));
        }

        else if (strcmp(tok[0], "bitmap_set") == 0){
            int idx = atoi(&tok[1][2]);
            int bit_idx = atoi(tok[2]);
            bool value;
            if (strcmp(tok[3], "true") == 0){
                value = true;
            }
            else{
                value = false;
            }
            bitmap_set(bitmap_arr[idx], bit_idx, value);
        }

        else if (strcmp(tok[0], "bitmap_set_multiple") == 0){
            int idx = atoi(&tok[1][2]);
            int bit_idx = atoi(tok[2]);
            int cnt = atoi(tok[3]);
            bool value;
            if (strcmp(tok[4], "true") == 0){
                value = true;
            }
            else{
                value = false;
            }
            bitmap_set_multiple(bitmap_arr[idx], bit_idx, cnt, value);
        }

        else if (strcmp(tok[0], "bitmap_set_all") == 0){
            int idx = atoi(&tok[1][2]);
            bool value;
            if (strcmp(tok[2], "true") == 0){
                value = true;
            }
            else{
                value = false;
            }
            bitmap_set_all(bitmap_arr[idx], value);
        }

        else if (strcmp(tok[0], "bitmap_scan") == 0){
            int idx = atoi(&tok[1][2]);
            int bit_idx = atoi(tok[2]);
            int cnt = atoi(tok[3]);
            bool value;
            if (strcmp(tok[4], "true") == 0){
                value = true;
            }
            else{
                value = false;
            }
            unsigned long result = bitmap_scan(bitmap_arr[idx], bit_idx, cnt, value);
            printf("%lu\n", result);
        }

        else if (strcmp(tok[0], "bitmap_scan_and_flip") == 0){
            int idx = atoi(&tok[1][2]);
            int bit_idx = atoi(tok[2]);
            int cnt = atoi(tok[3]);
            bool value;
            if (strcmp(tok[4], "true") == 0){
                value = true;
            }
            else{
                value = false;
            }
            unsigned long result = bitmap_scan_and_flip(bitmap_arr[idx], bit_idx, cnt, value);
            printf("%lu\n", result);
        }

        else if (strcmp(tok[0], "bitmap_reset") == 0){
            int idx = atoi(&tok[1][2]);
            int bit_idx = atoi(tok[2]);
            bitmap_reset(bitmap_arr[idx], bit_idx);
        }

        else if (strcmp(tok[0], "bitmap_none") == 0){
            int idx = atoi(&tok[1][2]);
            int bit_idx = atoi(tok[2]);
            int cnt = atoi(tok[3]);
            if (bitmap_none(bitmap_arr[idx], bit_idx, cnt)){
                printf("true\n");
            }
            else{
                printf("false\n");
            }
        }

        else if (strcmp(tok[0], "bitmap_flip") == 0){
            int idx = atoi(&tok[1][2]);
            int bit_idx = atoi(tok[2]);
            bitmap_flip(bitmap_arr[idx], bit_idx);
        }

        else if (strcmp(tok[0], "bitmap_expand") == 0){
            int idx = atoi(&tok[1][2]);
            int size = atoi(tok[2]);
            bitmap_expand(bitmap_arr[idx], size);
        }

        else if (strcmp(tok[0], "bitmap_dump") == 0){
            int idx = atoi(&tok[1][2]);
            bitmap_dump(bitmap_arr[idx]);
        }

        else if (strcmp(tok[0], "bitmap_count") == 0){
            int idx = atoi(&tok[1][2]);
            int bit_idx = atoi(tok[2]);
            int cnt = atoi(tok[3]);
            bool value;
            if (strcmp(tok[4], "true") == 0){
                value = true;
            }
            else{
                value = false;
            }
            printf("%zu\n",bitmap_count(bitmap_arr[idx], bit_idx, cnt, value));
        }

        else if (strcmp(tok[0], "bitmap_contains") == 0){
            int idx = atoi(&tok[1][2]);
            int bit_idx = atoi(tok[2]);
            int cnt = atoi(tok[3]);
            bool value;
            if (strcmp(tok[4], "true") == 0){
                value = true;
            }
            else{
                value = false;
            }
            
            if (bitmap_count(bitmap_arr[idx], bit_idx, cnt, value)){
                printf("true\n");
            }
            else{
                printf("false\n");
            }
        }

        else if (strcmp(tok[0], "bitmap_any") == 0){
            int idx = atoi(&tok[1][2]);
            int bit_idx = atoi(tok[2]);
            int cnt = atoi(tok[3]);
            
            if (bitmap_any(bitmap_arr[idx], bit_idx, cnt)){
                printf("true\n");
            }
            else{
                printf("false\n");
            }
        }

        else if (strcmp(tok[0], "bitmap_all") == 0){
            int idx = atoi(&tok[1][2]);
            int bit_idx = atoi(tok[2]);
            int cnt = atoi(tok[3]);
            
            if (bitmap_all(bitmap_arr[idx], bit_idx, cnt)){
                printf("true\n");
            }
            else{
                printf("false\n");
            }
        }

        else{
            printf("%s: UNALLOWED COMMAND\n", tok[0]);
        }
    }
    return 0;
}
