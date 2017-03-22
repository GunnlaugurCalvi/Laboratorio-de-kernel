#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>

#include "pidinfo.h"

/* Part I */
void run_current(void)
{
        pid_t pid = 0;
	int fd;


	/* Your code here */
	fd = open("/dev/kernellab1", O_RDONLY);
	read(fd, &pid, sizeof(pid));
	close(fd);

        printf("ANS: Current PID: %d\n", pid);
}

/* Part II */
void run_pid(pid_t pid)
{
	int fd;
        struct pid_info info;
	struct kernellab_message kl_message;
	memset(&info, 0, sizeof(struct pid_info));
	kl_message.pid = pid;
	kl_message.address = &info;
	
	
	fd = open("/dev/kernellab2", O_WRONLY);
	write(fd, &kl_message, sizeof(kl_message));
	close(fd);

        printf("ANS: PID: %d\n", info.pid);
        printf("ANS: COMM: %s\n", info.comm);
        printf("ANS: State: %ld\n", info.state);
}

/* Part III */
void run_sysfs(void)
{
	int fd_current;
	int fd_pid;
	int fd_all;

	int fd_dev_current;
	int fd_dev_pid;
	
	int all_count = 0;
	int current_count = 0;
	int pid_count = 0;

	char buf[1024]; /* Can be used for the contents of the sysfs files */

	fd_dev_current = open("/dev/kernellab1", O_RDONLY);
	fd_dev_pid = open("/dev/kernellab2", O_RDONLY);	

	fd_current = open("/sys/kernel/kernellab/current_count", O_RDONLY);
	fd_pid = open("/sys/kernel/kernellab/pid_count", O_RDONLY);
	fd_all = open("/sys/kernel/kernellab/all_count", O_RDONLY);



	read(fd_current, buf, sizeof(buf));
	current_count = atoi(buf);

	read(fd_pid, buf, sizeof(buf));
	pid_count = atoi(buf);
	
	read(fd_all, buf, sizeof(buf));	
	all_count = atoi(buf);
	
	printf("ANS: ALL_COUNT: %d\n", all_count);
	printf("ANS: CURRENT_COUNT: %d\n", current_count);
	printf("ANS: PID_COUNT: %d\n\n", pid_count);

	fd_current = open("/sys/kernel/kernellab/current_count", O_RDONLY);
	fd_pid = open("/sys/kernel/kernellab/pid_count", O_RDONLY);
	fd_all = open("/sys/kernel/kernellab/all_count", O_RDONLY);


	ioctl(fd_dev_current, RESET);	
	
	read(fd_current, buf, sizeof(buf));
	current_count = atoi(buf);

	read(fd_pid, buf, sizeof(buf));
	pid_count = atoi(buf);
	
	read(fd_all, buf, sizeof(buf));	
	all_count = atoi(buf);
	
	
	printf("ANS: ALL_COUNT: %d\n", all_count);
	printf("ANS: CURRENT_COUNT: %d\n", current_count);
	printf("ANS: PID_COUNT: %d\n\n", pid_count);

	fd_current = open("/sys/kernel/kernellab/current_count", O_RDONLY);
	fd_pid = open("/sys/kernel/kernellab/pid_count", O_RDONLY);
	fd_all = open("/sys/kernel/kernellab/all_count", O_RDONLY);

	
	ioctl(fd_dev_pid, RESET);
	
	read(fd_current, buf, sizeof(buf));
	current_count = atoi(buf);

	read(fd_pid, buf, sizeof(buf));
	pid_count = atoi(buf);
	
	read(fd_all, buf, sizeof(buf));	
	all_count = atoi(buf);
		
	
	printf("ANS: ALL_COUNT: %d\n", all_count);
	printf("ANS: CURRENT_COUNT: %d\n", current_count);
	printf("ANS: PID_COUNT: %d\n\n", pid_count);

	
	close(fd_dev_current);
	close(fd_dev_pid);
	close(fd_current);
	close(fd_pid);
	close(fd_all);
}

int main()
{
	pid_t pid;
	printf("=== Part I ===\n");
        run_current();
	printf("\n");

	printf("=== Part II ===\n");
	if (!(pid = fork())) {
		while (1);
	}
	run_pid(pid);
	kill(pid, SIGKILL);
	printf("\n");

	printf("=== Part III ===\n");
	run_sysfs();
        return EXIT_SUCCESS;
}
