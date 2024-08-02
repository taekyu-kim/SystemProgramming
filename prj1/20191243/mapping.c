#include "mapping.h"

struct list *create_list(char *tok){
    int idx = atoi(&tok[4]);
    if (list_arr[idx] != NULL){
        printf("CAN'T CREATE: %s ALREADY CREATED\n", tok);
        return NULL;
    }
    else{
        struct list *new_list = (struct list *)malloc(sizeof(struct list));
        list_init(new_list);
        list_arr[idx] = new_list;
        return new_list;
    }
}

bool delete_list(char *tok){
    int idx = atoi(&tok[4]);
    if (list_arr[idx] == NULL){ // create 된 적이 없는 list라면
        return false;
    }
    else{
        free(list_arr[idx]);
        list_arr[idx] = NULL;
        return true;
    }
}

void dumpdata_list(char *tok){
    int idx = atoi(&tok[4]);
    if (list_arr[idx] == NULL){
        printf("CAN'T DUMPDATA: %s NOT CREATED\n", tok);
        return;
    }
    else{
        int flag = 0;
        struct list_elem *temp = list_begin(list_arr[idx]);
        while (temp->next != NULL){
            int dump = list_entry(temp, struct list_item, elem)->data;
            printf("%d ", dump);
            temp = list_next(temp);
            flag = 1;
        }
        if (flag){
            printf("\n");
        }
        return;
    }
}

struct hash *create_hash(char *tok){
    int idx = atoi(&tok[4]);
    if (hash_arr[idx] != NULL){
        printf("CAN'T CREATE: %s ALREADY CREATED\n", tok);
        return NULL;
    }
    else{
        void *aux = NULL;
        struct hash *new_hash = (struct hash *)malloc(sizeof(struct hash));
        hash_init(new_hash, hash_func, hash_less, aux);
        hash_arr[idx] = new_hash;
        return new_hash;
    }
}

bool delete_hash(char *tok){
    int idx = atoi(&tok[4]);
    if (hash_arr[idx] == NULL){ // create 된 적이 없는 hashtable이라면
        return false;
    }
    else{
        hash_destroy(hash_arr[idx], destructor);
        hash_arr[idx] = NULL;
        return true;
    }
}

void dumpdata_hash(char *tok){
    int idx = atoi(&tok[4]);
    if (hash_arr[idx] == NULL){
        printf("CAN'T DUMPDATA: %s NOT CREATED\n", tok);
        return;
    }
    else{
        int flag = 0;
        struct hash_iterator *iter = (struct hash_iterator *)malloc(sizeof(struct hash_iterator));
        hash_first(iter, hash_arr[idx]);
        hash_next(iter);
        struct list_elem *temp = &hash_cur(iter)->list_elem;
        while (temp != NULL){
            int dump = list_elem_to_hash_elem(temp)->value;
            printf("%d ", dump);
            temp = &hash_next(iter)->list_elem;
            flag = 1;
        }
        if (flag){
            printf("\n");
        }
        return;
    }
}

struct bitmap *create_bitmap(char *tok1, char *tok2){
    int idx = atoi(&tok1[2]);
    int bit_cnt = atoi(tok2);
    if (bitmap_arr[idx] != NULL){
        printf("CAN'T CREATE: %s ALREADY CREATED\n", tok1);
        return NULL;
    }
    else{
        struct bitmap *new_bitmap = bitmap_create(bit_cnt);
        bitmap_arr[idx] = new_bitmap;
        return new_bitmap;
    }
}

bool delete_bitmap(char *tok){
    int idx = atoi(&tok[2]);
    if (bitmap_arr[idx] == NULL){ // create 된 적이 없는 bitmap이라면
        return false;
    }
    else{
        bitmap_destroy(bitmap_arr[idx]);
        bitmap_arr[idx] = NULL;
        return true;
    }
}

void dumpdata_bitmap(char *tok){
    int idx = atoi(&tok[2]);
    struct bitmap *bitmap = bitmap_arr[idx];
    if (bitmap == NULL){
        printf("CAN'T DUMPDATA: %s NOT CREATED\n", tok);
        return;
    }
    else{
        for (int i = 0; i < bitmap->bit_cnt; i++){
             printf("%ld", (bitmap->bits[i / (sizeof(elem_type) * 8)] >> (i % (sizeof(elem_type) * 8))) & 1);
        }
        printf("\n");
        return;
    }
}