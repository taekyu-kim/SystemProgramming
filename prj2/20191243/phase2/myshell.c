/* Headers */
#include "myshell.h"

/**************** Implementation *****************/
int main() {
	char cmdline[MAXLINE];	

	init_queue();
	Signal(SIGCHLD, sigchld_handler);
	Signal(SIGINT, sigint_handler);
	Signal(SIGTSTP, sigtstp_handler);

	while (1) {						
		read_flag = 0;

		Sio_puts("CSE4100-SP-P2> ");
		Fgets(cmdline, MAXLINE, stdin);
		if (feof(stdin)) exit(0);
		read_flag = 1;
		
		for (int i = 0; cmdline[i]; i++) {
			if (cmdline[i] == '&' && cmdline[i + 1] != '&' && cmdline[i - 1] != '&') {
				cmdline[i] = '\0';									
				strcat(cmdline, " &\n");
				break;
			}
		}

		pipe_flag = is_pipe(cmdline);
		eval(cmdline, 0, 0);
	}

	return 0;
}

void eval(char *cmdline, int pipe_fd, int cnt) {
	char *argv[MAXARGS];
	char buf[MAXLINE];
	pid_t pid;
	sigset_t mask_all, mask1, mask2, prev;
	int bg, index, fd[2], last_flag, temp_fd;

	
	Sigfillset(&mask_all);
	Sigemptyset(&mask1); Sigemptyset(&mask2);
	Sigaddset(&mask1, SIGCHLD);	Sigaddset(&mask1, SIGINT); Sigaddset(&mask1, SIGTSTP);
	Sigaddset(&mask2, SIGINT); Sigaddset(&mask2, SIGTSTP);

	if (pipe_flag) {
		Pipe(fd);
	}

	last_flag = cmd_buffer(cmdline, buf, &index);
	parseline(buf, argv);
	bg = is_background(cmdline);
	
	if (!argv[0]) {
		return;
	}
	
	if (!builtin_command(argv)) {	// check if argv is the built-in-command such as cd
		if (bg) {
			Sigprocmask(SIG_BLOCK, &mask1, &prev);
		}

		/* Child Process */
		if ((pid = Fork()) == 0) {							
			Sigprocmask(SIG_SETMASK, &prev, NULL);
			Sigprocmask(SIG_BLOCK, &mask2, &prev);
															
			if (!last_flag) {		// if this cmd is not last cmd
				Close(fd[0]);
				Dup2(pipe_fd, STDIN_FILENO);
				Dup2(fd[1], STDOUT_FILENO);	
			}
			else {					// if this cmd is the last cmd
				Dup2(pipe_fd, STDIN_FILENO);
			}

			Execvp(argv[0], argv);
		}

		/* Parent Process */
		if (!bg) {	/* Foreground Process */
			Sigprocmask(SIG_BLOCK, &mask_all, &prev);
			Enqueue(pid, 1, cmdline);		
													
			PID = 0;
			while (!PID) {							
				if (is_foreground()) {			
					Sigsuspend(&prev);
				}
				else {
					break;
				}
			}
		}

		else {		/* Background Process */
			Job *temp;	
			Sigprocmask(SIG_BLOCK, &mask_all, NULL);
			Enqueue(pid, 2, cmdline);		
			temp = Search_job(pid);

			if (!cnt) {
				Sio_printBGjob(temp->idx, pid);	
			}
		}

		/* Both Foreground and Background */
		if (!last_flag) {							
			Close(fd[1]);							
			Close(pipe_fd);
		}
		else {
			temp_fd = (!cnt) ? 1 : 0;
			Dup2(STDIN_FILENO, pipe_fd);		
			Dup2(STDOUT_FILENO, temp_fd);		
		}
		Sigprocmask(SIG_SETMASK, &prev, NULL);

		if (!last_flag)
			eval(1 + index + cmdline, fd[0], cnt + 1);	
	}
	return;
}

int cmd_buffer(char *cmdline, char *buf, int *index) {		
	int i, j;
	for (i = 0; cmdline[i]; i++) {
		if (cmdline[i] == '|') {				
			buf[i] = '\0';
			*index = i;
			return 0;
		}
		else if ((cmdline[i] == '\"') || (cmdline[i] == '\'')) {	
			for (j = i + 1; (cmdline[j] != '\"' && cmdline[j] != '\'') && cmdline[j]; j++) {
				if (cmdline[j] == ' ') {
					cmdline[j] = -1;
				}
			}
			cmdline[i] = ' ';
			cmdline[j] = ' ';
		}
		buf[i] = cmdline[i];
	}
	buf[i] = '\0';
	*index = i;
	return 1;
}

int parseline(char *buf, char **argv) {		
	char *delim;							// points to first space delimiter
	int argc, bg;							// number of args and background flag

	if (buf[strlen(buf) - 1] != '\n') {
		buf[strlen(buf)] = ' ';
	}
	else {
		buf[strlen(buf) - 1] = ' ';			// replace trailing '\n' or ' ' with space
	}

	while (*buf && (*buf == ' '))			// ignore leading spaces
		buf++;

	argc = 0;
	while ((delim = strchr(buf, ' '))) {	// build the argv list
		argv[argc] = buf;
		*delim = '\0';

		for (int i = 0; argv[argc][i]; i++) {	// replace temp value -1
			if (argv[argc][i] == -1)				// with space! (see 'buffering' func)
				argv[argc][i] = ' ';
		}
		argc++;

		buf = delim + 1;
		while (*buf && (*buf == ' '))		// ignore spaces
			buf++;
	}
	argv[argc] = NULL;

	if (!argc)							// ignore blank line
		return 1;

	if ((bg = (*argv[argc - 1] == '&'))) // should the job run in the background?
		argv[--argc] = NULL;

	return bg;
}

int is_pipe(char *cmdline) {		
	for (int i = 0; cmdline[i]; i++) {
		if (cmdline[i] == '|') {
			return 1;
		}
	}
	return 0;
}

int is_background(char *cmdline) {			
	for (int i = 0; cmdline[i]; i++) {
		if (cmdline[i] == '&' && cmdline[i + 1] != '&' && cmdline[i - 1] != '&') 
			return 1;					
	}
	return 0;
}

int is_foreground() {				
	for (int i = 1; i < MAXJOB; i++) {
		if (job_queue[i].state == 1)
			return 1;
	}
	return 0;
}




/* Built-In-Commands */
int builtin_command(char **argv) {	 
	if (!strcmp(argv[0], "cd"))			
		return cd_command(argv);
	if (!strcmp(argv[0], "&"))			
		return 1;
	if (!strcmp(argv[0], "jobs"))
		return jobs_command();
	if (!strcmp(argv[0], "fg"))		
		return fg_command(argv);
	if (!strcmp(argv[0], "bg"))			
		return bg_command(argv);
	if (!strcmp(argv[0], "kill"))		
		return kill_command(argv);
	if (!strcmp(argv[0], "exit"))		
		exit(0);
	if (!strcmp(argv[0], "quit"))		
		exit(0);

	return 0;						
}



int cd_command(char **argv) {			
	if (argv[1] == NULL || strcmp(argv[1], "~") == 0)	
		return chdir(getenv("HOME")) + 1;
	if (chdir(argv[1]) < 0)							
		printf("No such directory\n");

	return 1;
}


int jobs_command(void) {					
	for (int i = 1; i < MAXJOB; i++) {
		if (job_queue[i].pid) {
			printf("[%d] ", i);

			if (job_queue[i].state == 2) {
				printf("Running %s\n", job_queue[i].cmdline);
				continue;
			}
			else if (job_queue[i].state == 3) {
				printf("Stopped %s\n", job_queue[i].cmdline);
				continue;
			}
			else {
				;
			}
		}
	}
	return 1;
}


int fg_command(char **argv) {
	int idx;
	pid_t target_pid;
	sigset_t mask, prev;

	Sigemptyset(&mask);
	Sigaddset(&mask, SIGCHLD);			
	Sigprocmask(SIG_BLOCK, &mask, &prev);

	if (argv[1] == NULL || argv[1][0] != '%') {		
		printf("bash: fg: current: no such job\n");
		return 1;
	}
	argv[1] = 1 + &(argv[1][0]);
	idx = atoi(argv[1]);
	if (!(idx > 0 && idx < MAXJOB)) { 
		printf("bash: fg: %d: no such job\n", idx);
		return 1;
	}
	target_pid = job_queue[idx].pid;
	if (job_queue[idx].state == 0) {	
		printf("bash: fg: %d: no such job\n", idx);
		return 1;
	}

	Sio_printjob(idx, job_queue[idx].cmdline);
	Kill(target_pid, SIGCONT);
	Change_state(target_pid, 1);

	PID = 0;
	while (!PID) {						
		if (is_foreground()) 
			Sigsuspend(&prev);
		else
			break;
	}
	Sigprocmask(SIG_SETMASK, &prev, NULL);
	return 1;
}

int bg_command(char **argv) {
	int idx;
	pid_t target_pid;
	sigset_t mask, prev;

	Sigemptyset(&mask);
	Sigaddset(&mask, SIGCHLD);		
	Sigprocmask(SIG_BLOCK, &mask, &prev);

	if (!argv[1] || argv[1][0] != '%') {		
		printf("bash: bg: current: no such job\n");
		return 1;
	}

	argv[1] = 1 + &(argv[1][0]);
	idx = atoi(argv[1]);
	if (!(idx > 0 && idx < MAXJOB)) {	
		printf("bash: bg: %d: no such job\n", idx);
		return 1;
	}
	target_pid = job_queue[idx].pid;
	if (job_queue[idx].state == 0) {	
		printf("bash: bg: %d: no such job\n", idx);
		return 1;
	}

	Sio_printjob(idx, job_queue[idx].cmdline);	
	Kill(target_pid, SIGCONT);						
	Change_state(target_pid, 2);

	Sigprocmask(SIG_SETMASK, &prev, NULL);
	return 1;		
}



int kill_command(char **argv) {
	int idx;
	pid_t target_pid;
	sigset_t mask, prev;

	Sigemptyset(&mask);
	Sigaddset(&mask, SIGCHLD);		
	Sigprocmask(SIG_BLOCK, &mask, &prev);

	if (argv[1] == NULL || argv[1][0] != '%') {		
		printf("kill: usage: kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]\n");
		return 1;
	}

	argv[1] = 1 + &(argv[1][0]);
	idx = atoi(argv[1]);
	if (!(idx > 0 && idx < MAXJOB)) {	
		printf("bash: kill: %d: no such job\n", idx);
		return 1;
	}
	target_pid = job_queue[idx].pid;
	if (job_queue[idx].state == 0) {	
		printf("bash: kill: %d: no such job\n", idx);
		return 1;
	}

	Kill(target_pid, SIGKILL);			
	printf("[%d] Terminated %s\n", idx, job_queue[idx].cmdline);
	Dequeue(target_pid);

	Sigprocmask(SIG_SETMASK, &prev, NULL);
	return 1;
}




/* Signal Handler */
void sigchld_handler(int sig) {				
	int olderrno = errno;
	int status;
	sigset_t mask_all, prev;

	Sigfillset(&mask_all);					
	while ((PID = waitpid(-1, &status, WNOHANG)) > 0) {		
		Sigprocmask(SIG_BLOCK, &mask_all, &prev);
											
		if (status == SIGPIPE || WIFEXITED(status)) 
			Dequeue(PID);					

		Sigprocmask(SIG_SETMASK, &prev, NULL);
	}

	if (!((PID == 0) || (PID == -1 && errno == ECHILD)))
		Sio_error("waitpid error");
	errno = olderrno;
}


void sigint_handler(int sig) {			
	int olderrno = errno;
	sigset_t mask_all, prev;
	Sigfillset(&mask_all);
	Sigprocmask(SIG_BLOCK, &mask_all, &prev);

	if (!read_flag) {
		Sio_puts("\nCSE4100-SP-P2> ");	
	}
	else {
		Sio_puts("\n");	
	}
	
	for (int i = 1; i < MAXJOB; i++) {
		if (job_queue[i].state == 1) {		
			Kill(job_queue[i].pid, SIGKILL);			
			Dequeue(job_queue[i].pid);
			
		}
	}
	Sigprocmask(SIG_SETMASK, &prev, NULL);

	errno = olderrno;
}


void sigtstp_handler(int sig) {			
	int olderrno = errno;
	sigset_t mask_all, prev;
	Sigfillset(&mask_all);
	Sigprocmask(SIG_BLOCK, &mask_all, &prev);

	Sio_puts("\n");
	for (int i = 1; i < MAXJOB; i++) {
		if (job_queue[i].state == 1) {		
			Kill(job_queue[i].pid, SIGSTOP);		
			job_queue[i].state = 3;
		}
	}
	Sigprocmask(SIG_SETMASK, &prev, NULL);

	errno = olderrno;
}