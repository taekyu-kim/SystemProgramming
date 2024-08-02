#include "list.h"
#include "hash.h"
#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct list *create_list(char *);
bool delete_list(char *);
void dumpdata_list(char *);

struct hash *create_hash(char *);
bool delete_hash(char *);
void dumpdata_hash(char *);

struct bitmap *create_bitmap(char *, char *);
bool delete_bitmap(char *);
void dumpdata_bitmap(char *);