/* $begin myshell.h */
#ifndef __MYSHELL_H__
#define __MYSHELL_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>


#define	MAXLINE	 8192
#define MAXARGS	 128
#define MAXJOB	 50


void unix_error(char *msg);
void app_error(char *msg);

char *Fgets(char *ptr, int n, FILE *stream);

pid_t Fork(void);
void Execvp(const char *filename, char *const argv[]);
pid_t Wait(int *status);
pid_t Waitpid(pid_t pid, int *iptr, int options);
void Kill(pid_t pid, int signum);

/* Signal */
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
void Sigemptyset(sigset_t *set);
void Sigfillset(sigset_t *set);
void Sigaddset(sigset_t *set, int signum);
int Sigsuspend(const sigset_t *set);

/* Sio */
ssize_t Sio_puts(char s[]);
ssize_t Sio_putl(long v);
ssize_t Sio_printjob(int idx, char *cmdline);
ssize_t Sio_printBGjob(int idx, pid_t pid);
void Sio_error(char s[]);

void Close(int fd);
int Dup2(int fd1, int fd2);
int Pipe(int *fd);

void unix_error(char *msg) /* Unix-style error */
{
	fprintf(stderr, "%s: %s\n", msg, strerror(errno));
	exit(0);
}

void app_error(char *msg) /* Application error */
{
	fprintf(stderr, "%s\n", msg);
	exit(0);
}

char *Fgets(char *ptr, int n, FILE *stream)
{
	char *rptr;
	
	if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
		app_error("Fgets error");

	return rptr;
}

pid_t Fork(void)
{
	pid_t pid;

	if ((pid = fork()) < 0)
		unix_error("Fork error");
	return pid;
}

void Execvp(const char *filename, char *const argv[])
{
	if (execvp(filename, argv) < 0) {
		printf("%s: Command not found.\n", filename);
		exit(0);
	}
}

pid_t Wait(int *status)
{
	pid_t pid;

	if ((pid = wait(status)) < 0)
		unix_error("Wait error");
	return pid;
}

pid_t Waitpid(pid_t pid, int *iptr, int options)
{
	pid_t retpid;

	if ((retpid = waitpid(pid, iptr, options)) < 0)
		Sio_puts("Waitpid error");

	return(retpid);
}

void Kill(pid_t pid, int signum)
{
	int rc;

	if ((rc = kill(pid, signum)) < 0)
		unix_error("Kill error");
}

handler_t *Signal(int signum, handler_t *handler)
{
	struct sigaction action, old_action;

	action.sa_handler = handler;
	sigemptyset(&action.sa_mask); /* Block sigs of type being handled */
	action.sa_flags = SA_RESTART; /* Restart syscalls if possible */

	if (sigaction(signum, &action, &old_action) < 0)
		unix_error("Signal error");
	return (old_action.sa_handler);
}

void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
	if (sigprocmask(how, set, oldset) < 0)
		unix_error("Sigprocmask error");
	return;
}

void Sigemptyset(sigset_t *set)
{
	if (sigemptyset(set) < 0)
		unix_error("Sigemptyset error");
	return;
}

void Sigfillset(sigset_t *set)
{
	if (sigfillset(set) < 0)
		unix_error("Sigfillset error");
	return;
}

void Sigaddset(sigset_t *set, int signum)
{
	if (sigaddset(set, signum) < 0)
		unix_error("Sigaddset error");
	return;
}

int Sigsuspend(const sigset_t *set)
{
	int rc = sigsuspend(set); /* always returns -1 */
	if (errno != EINTR)
		unix_error("Sigsuspend error");
	return rc;
}

static void sio_reverse(char s[])
{
	int c, i, j;

	for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

static void sio_ltoa(long v, char s[], int b)
{
	int c, i = 0;

	do {
		s[i++] = ((c = (v % b)) < 10) ? c + '0' : c - 10 + 'a';
	} while ((v /= b) > 0);
	s[i] = '\0';
	sio_reverse(s);
}

static size_t sio_strlen(char s[])
{
	int i = 0;

	while (s[i] != '\0')
		++i;
	return i;
}

ssize_t sio_puts(char s[]) /* Put string */
{
	return write(STDOUT_FILENO, s, sio_strlen(s));
}

ssize_t sio_putl(long v) /* Put long */
{
	char s[128];

	sio_ltoa(v, s, 10); /* Based on K&R itoa() */
	return sio_puts(s);
}

void sio_error(char s[]) /* Put error message and exit */
{
	sio_puts(s);
	_exit(1);
}

ssize_t Sio_putl(long v)
{
	ssize_t n;

	if ((n = sio_putl(v)) < 0)
		sio_error("Sio_putl error");
	return n;
}

ssize_t Sio_puts(char s[])
{
	ssize_t n;

	if ((n = sio_puts(s)) < 0)
		sio_error("Sio_puts error");
	return n;
}

ssize_t Sio_printjob(int idx, char *cmdline) {
	Sio_puts("["); Sio_putl((long)idx); Sio_puts("] running ");
	Sio_puts(cmdline); Sio_puts("\n");
}

ssize_t Sio_printBGjob(int idx, pid_t pid) {
	Sio_puts("["); Sio_putl((long)idx);
	Sio_puts("] "); Sio_putl((long)pid); Sio_puts("\n");
}

void Sio_error(char s[])
{
	sio_error(s);
}

void Close(int fd)
{
	int rc;

	if ((rc = close(fd)) < 0)
		unix_error("Close error");
}

int Dup2(int fd1, int fd2)
{
	int rc;

	if ((rc = dup2(fd1, fd2)) < 0)
		unix_error("Dup2 error");
	return rc;
}

int Pipe(int *fd)
{
	if (pipe(fd) < 0)
		unix_error("Pipe error");
}



/* Types of Job's State */
typedef struct {
	int idx;
	pid_t pid;
	int state;		// 0: Invalid, 1: Foreground, 2: Background, 3: Stopped
	char cmdline[MAXLINE];
	int is_last;
} Job;

Job job_queue[MAXJOB];
int queue_last;
int queue_size;

void init_queue(void);
void Enqueue(pid_t pid, int state, char *cmdline);
void Dequeue(pid_t pid);
void Change_state(pid_t pid, int state);
Job *Search_job(pid_t pid);

void init_queue(void) {			
	for (int i = 1; i < MAXJOB; i++) {
		job_queue[i].idx = 0;
		job_queue[i].pid = 0;
		job_queue[i].state = 0;
		job_queue[i].cmdline[0] = '\0';
		job_queue[i].is_last = 0;
	}
}

void Enqueue(pid_t pid, int state, char *cmdline) {
	int i;

	if (!queue_size) {
		i = 1;
		queue_last = i;
	}
	else {
		if ((queue_last + 1) >= MAXJOB)
			unix_error("Enqueue error");

		job_queue[queue_last].is_last = 0;
		queue_last++;
		i = queue_last;
	}

	job_queue[i].idx = i;
	job_queue[i].pid = pid;
	job_queue[i].state = state;
	job_queue[i].cmdline[0] = cmdline[0];
	
	int j = 1;
	for (int k = 1; cmdline[k]; j++, k++) {						
		if (cmdline[k] == '\n' || cmdline[k] == '&' 
		|| (cmdline[k - 1] == ' ' && cmdline[k] == ' ')) {
			j--;
			continue;
		}
		job_queue[i].cmdline[j] = cmdline[k];
	}
	job_queue[i].cmdline[j] = '\0';
	job_queue[i].is_last = 1;
	queue_size++;
}

void Dequeue(pid_t pid) {
	for (int i = 1; i < MAXJOB; i++) {
		if (job_queue[i].pid == pid && job_queue[i].state != 0) {
			strcpy(job_queue[i].cmdline, "");
			job_queue[i].pid = 0;
			job_queue[i].idx = 0;
			job_queue[i].state = 0;

			if (job_queue[i].is_last)
				queue_last--;

			queue_size--;
			return;
		}
	}
}

void Change_state(pid_t pid, int state) {
	for (int i = 1; i < MAXJOB; i++) {
		if (job_queue[i].pid == pid) {
			job_queue[i].state = state;		
			return;
		}
	}
	unix_error("Change_state error");
}

Job *Search_job(pid_t pid) {			
	for (int i = 1; i < MAXJOB; i++) {
		if (job_queue[i].pid == pid)
			return &(job_queue[i]);			
	}
	return NULL;
}


volatile sig_atomic_t PID;
int read_flag;
int pipe_flag;

/* Read-Eval Cycle */
void eval(char *cmdline, int pipe_fd, int cnt);
int cmd_buffer(char *cmdline, char *buf, int *idx);
int parseline(char *buf, char **argv);

/* Built-In Commands */
int builtin_command(char **argv);
int cd_command(char **argv);
int jobs_command(void);
int fg_command(char **argv);
int bg_command(char **argv);
int kill_command(char **argv);

/* Flag */
int is_pipe(char *cmdline);
int is_background(char *cmdline);
int is_foreground(void);

/* Signal Handling */
void sigchld_handler(int sig);
void sigint_handler(int sig);
void sigtstp_handler(int sig);


#endif