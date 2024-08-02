/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */

#include "csapp.h"

#define MAX_ITEM	1000000			/* size of pointer array */
#define SBUFSIZE	1000
#define NTHREADS	1000

typedef enum {						
	show, buy, sell, exitt, errorr
} command;

typedef struct item {				
	int ID;							
	int left_stock;
	int price;
	int readcnt;
	sem_t mutex, w;
	struct item *right;				
	struct item *left;
} item;

typedef struct {
	int *buf;
	int n;
	int front;
	int rear;
	sem_t mutex;
	sem_t slots;
	sem_t items;
} sbuf_t;

sbuf_t sbuf;

void LoadStock();
void StoreStock();

command parseline(char *buf, int *id, int *num);
void OperateCmd(int connfd, char *buf);

void ShowStock(int connfd);
void BuyStock(int connfd, int id, int num);
void SellStock(int connfd, int id, int num);
void ExitServer(int connfd);
void PrintError(int connfd);


void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);


item *root = NULL;					
item *Tree[MAX_ITEM];				
int tree_size;						

void InsertItem(int ID, int left_stock, int price);
item* SearchTree(int ID);
void ClearTree(item* node);


/****************** SIGINT Handler ******************/
void sigint_handler(int sig) {
	int olderrno = errno;

	StoreStock();				
	ClearTree(root);			
	printf("\nServer has terminated with 'stock.txt' update!\n");
	exit(0);

	errno = olderrno;
}
/****************************************************/

/****************************************************/
void *thread(void *vargp) {
	Pthread_detach(pthread_self());

	while (1) {
		int n, connfd = sbuf_remove(&sbuf);
		char buf[MAXLINE];
		rio_t rio;

		Rio_readinitb(&rio, connfd);
		while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
			printf("server received %d bytes\n", n);
			OperateCmd(connfd, buf);
		}

		Close(connfd);
	}
} 
/****************************************************/

/******************** MAIN START ********************/
int main(int argc, char **argv) {
	int listenfd, connfd;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	char client_hostname[MAXLINE], client_port[MAXLINE];
	pthread_t tid;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}

	Signal(SIGINT, sigint_handler);			
	LoadStock();							
	
	listenfd = Open_listenfd(argv[1]);
	sbuf_init(&sbuf, SBUFSIZE);

	for (int i = 0; i < NTHREADS; i++) {
		Pthread_create(&tid, NULL, thread, NULL);
	}

	while (1) {
		clientlen = sizeof(struct sockaddr_storage);
		connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
		Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
		printf("Connected to (%s, %s)\n", client_hostname, client_port);
		sbuf_insert(&sbuf, connfd);
	}
}
/********************* MAIN END *********************/


/****************************************************/
void LoadStock() {
	FILE *fp;
	if (!(fp = fopen("stock.txt", "r"))) {
		fprintf(stderr, "stock.txt does not exist.\n");
		exit(0);
	}

    int id, left_stock, price;
	char line[32];
	while (Fgets(line, sizeof(line), fp)) {
		sscanf(line, "%d %d %d", &id, &left_stock, &price);
		InsertItem(id, left_stock, price);
	}
	Fclose(fp);
    return;
}

void StoreStock() {
	FILE *fp;
	if (!(fp = fopen("stock.txt", "w"))) {
		fprintf(stderr, "fopen error(StoreStock)\n");
		exit(0);
	}

	for (int i = 0; i < tree_size; i++) {
		char line[32];
		sprintf(line, "%d %d %d\n", Tree[i]->ID, Tree[i]->left_stock, Tree[i]->price);
		Fputs(line, fp);
	}
	Fclose(fp);
    return;
}

command parseline(char *buf, int *id, int *num) {
    char cmd[8];
	sscanf(buf, "%s %d %d", cmd, id, num);

	if (!strcmp("show", cmd))
		return show;
	else if (!strcmp("buy", cmd))
		return buy;
	else if (!strcmp("sell", cmd))
		return sell;
	else if (!strcmp("exit", cmd))
		return exitt;
	else
        return errorr;
}

void OperateCmd(int connfd, char *buf) {
    int ID, num;
    command cmd = parseline(buf, &ID, &num);
    if (cmd == show) {
        ShowStock(connfd);
    }
    else if (cmd == buy) {
        BuyStock(connfd, ID, num);
        return;
    }
    else if (cmd == sell) {
        SellStock(connfd, ID, num);
    }
    else if (cmd == exitt) {
        ExitServer(connfd);
    }
    else if (cmd == errorr) {
        PrintError(connfd);
    }
    return;
}

void ShowStock(int connfd) {
	char buf[MAXLINE] = {0};
    char *ptr = buf;

	for (int i = 0; i < tree_size; i++) {
		P(&(Tree[i]->mutex));
		(Tree[i]->readcnt)++;
		if (Tree[i]->readcnt == 1) {
			P(&(Tree[i]->w));
		}
		V(&(Tree[i]->mutex));

        ptr += sprintf(ptr, "%d %d %d\n", Tree[i]->ID, Tree[i]->left_stock, Tree[i]->price);

		P(&(Tree[i]->mutex));
		(Tree[i]->readcnt)--;
		if (Tree[i]->readcnt == 0) {
			V(&(Tree[i]->w));
		}
		V(&(Tree[i]->mutex));
	}
	Rio_writen(connfd, buf, MAXLINE);
}

void BuyStock(int connfd, int ID, int num) {
	item *temp = SearchTree(ID);
	P(&(temp->w));
	if (temp->left_stock < num) {
		V(&(temp->w));
        Rio_writen(connfd, "Not enough left stock\n", MAXLINE);
    }
	else {
		temp->left_stock -= num;
		V(&(temp->w));
        Rio_writen(connfd, "[buy] success\n", MAXLINE);
	}
}

void SellStock(int connfd, int ID, int num) {
	item *temp = SearchTree(ID);

	P(&(temp->w));
	temp->left_stock += num;
	V(&(temp->w));
	Rio_writen(connfd, "[sell] success\n", MAXLINE);
}

void ExitServer(int connfd) {
	Rio_writen(connfd, "exit", MAXLINE);
}

void PrintError(int connfd) {
	Rio_writen(connfd, "Invalid Command\n", MAXLINE);
}
/****************************************************/


/****************************************************/
void sbuf_init(sbuf_t *sp, int n) {
	sp->buf = Calloc(n, sizeof(int));		
	sp->n = n; 								
	sp->front = sp->rear = 0; 				
	Sem_init(&sp->mutex, 0, 1); 			
	Sem_init(&sp->slots, 0, n); 			
	Sem_init(&sp->items, 0, 0); 			
}

void sbuf_deinit(sbuf_t *sp) {
	Free(sp->buf);								
}

void sbuf_insert(sbuf_t *sp, int item) {
	P(&sp->slots); 								
	P(&sp->mutex); 								
	sp->buf[(++sp->rear) % (sp->n)] = item;		
	V(&sp->mutex); 								
	V(&sp->items); 							
}

int sbuf_remove(sbuf_t *sp) {
	int item;
	P(&sp->items); 
	P(&sp->mutex); 
	item = sp->buf[(++sp->front) % (sp->n)];
	V(&sp->mutex);
	V(&sp->slots); 							

	return item;
}
/****************************************************/


/****************************************************/
item* CreateItem(int ID, int left_stock, int price) {
    item* new_item = (item*)malloc(sizeof(item));
    new_item->ID = ID;
    new_item->left_stock = left_stock;
    new_item->price = price;
    new_item->left = NULL;
    new_item->right = NULL;
	new_item->readcnt = 0;
	Sem_init(&(new_item->mutex), 0, 1);
	Sem_init(&(new_item->w), 0, 1);
    Tree[tree_size++] = new_item;
    return new_item;
}

void InsertItem(int ID, int left_stock, int price) {
	if (root == NULL) {
		root = CreateItem(ID, left_stock, price);
	}

    else {
        item* current = root;
        item* parent = NULL;
        while (current != NULL) {
            parent = current;
            if (ID <= current->ID) {
                current = current->left;
            }
            else {
                current = current->right;
            }
        }
        if (ID <= parent->ID) {
            parent->left = CreateItem(ID, left_stock, price);
        }
        else {
            parent->right = CreateItem(ID, left_stock, price);
        }
    }
}

item* SearchTree(int ID) {
    item* current = root;
	while (current != NULL) {
        if (ID == current->ID) {
            return current;
        }
        else if (ID <= current->ID) {
            current = current->left;
        }
        else {
            current = current->right;
        }
    }
    return NULL;
}

void ClearTree(item* Item) {
	if (Item != NULL) {
		ClearTree(Item->left);
		ClearTree(Item->right);
		Free(Item);
	}
}
/****************************************************/