#include "csapp.h"

#define MAX_ITEM	1000000

typedef enum {
	show, buy, sell, exitt, errorr
} command;

typedef struct item {
	int ID;
	int left_stock;
	int price;
	struct item *right;
	struct item *left;
} item;

typedef struct pool {
	int maxfd;
	fd_set read_set;
	fd_set ready_set;
	int nready;
	int maxi;
	int clientfd[FD_SETSIZE];
	rio_t clientrio[FD_SETSIZE];
} pool;

void LoadStock();
void StoreStock();

command parseline(char *buf, int *id, int *num);
void OperateCmd(int connfd, char *buf);

void ShowStock(int connfd);
void BuyStock(int connfd, int id, int num);
void SellStock(int connfd, int id, int num);
void ExitServer(int connfd);
void PrintError(int connfd);


void init_pool(int listenfd, pool *p);
void add_client(int connfd, pool *p);
void check_client(pool *p);


item *root = NULL;
item *Tree[MAX_ITEM];
int tree_size;

void InsertItem(int ID, int left_stock, int price);
item* SearchTree(int ID);
void ClearTree(item* node);


/***************** SIGINT Handler *****************/
void sigint_handler(int sig) {
	int olderrno = errno;

	StoreStock();
	ClearTree(root);
	printf("\nServer has terminated with 'stock.txt' update!\n");
	exit(0);

	errno = olderrno;
}
/**************************************************/

/******************** MAIN START ********************/
int main(int argc, char **argv) {
	int listenfd, connfd;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	char client_hostname[MAXLINE], client_port[MAXLINE];
	static pool pool;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}

	Signal(SIGINT, sigint_handler);			// install the SIGINT handler
	LoadStock();							// load the 'stock.txt', and construct tree
	
	listenfd = Open_listenfd(argv[1]);
	init_pool(listenfd, &pool);				// initialize the pool for I/O Multiplexing

	while (1) {
		pool.ready_set = pool.read_set;
		pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);

		if (FD_ISSET(listenfd, &pool.ready_set)) {			// if pending at listenfd,
			clientlen = sizeof(struct sockaddr_storage);
			connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);

			Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
			printf("Connected to (%s, %s)\n", client_hostname, client_port);

			add_client(connfd, &pool);						// add new connfd to pool
		}

		check_client(&pool);				// check if there's any pendings at connfds
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
        ptr += sprintf(ptr, "%d %d %d\n", Tree[i]->ID, Tree[i]->left_stock, Tree[i]->price);
	}
	Rio_writen(connfd, buf, MAXLINE);
}

void BuyStock(int connfd, int ID, int num) {
	item *temp = SearchTree(ID);

	if (temp->left_stock < num) {
        Rio_writen(connfd, "Not enough left stock\n", MAXLINE);
    }
	else {
		temp->left_stock -= num;
        Rio_writen(connfd, "[buy] success\n", MAXLINE);
	}
}

void SellStock(int connfd, int ID, int num) {
	item *temp = SearchTree(ID);
	temp->left_stock += num;
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
void init_pool(int listenfd, pool *p) {
	p->maxi = -1;
	for (int i = 0; i < FD_SETSIZE; i++)
		p->clientfd[i] = -1;

	p->maxfd = listenfd;
	FD_ZERO(&p->read_set);
	FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool *p) {
	int i;
	p->nready--;

	for (i = 0; i < FD_SETSIZE; i++) {
		if (p->clientfd[i] < 0) {
			p->clientfd[i] = connfd;
			Rio_readinitb(&p->clientrio[i], connfd);

			FD_SET(connfd, &p->read_set);

			if (connfd > p->maxfd)
				p->maxfd = connfd;
			if (i > p->maxi)
				p->maxi = i;

			break;
		}
	}

	if (i == FD_SETSIZE)
		app_error("Error in add_client!\n");
}

void check_client(pool *p) {
	int connfd, n;
	char buf[MAXLINE];
	rio_t rio;

	for (int i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
		connfd = p->clientfd[i];
		rio = p->clientrio[i];

		if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
            p->nready--;
			if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
				printf("server received %d bytes\n", n);
				OperateCmd(connfd, buf);
			}
			else {
				Close(connfd);
				FD_CLR(connfd, &p->read_set);
				p->clientfd[i] = -1;
			}
		}
	}
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