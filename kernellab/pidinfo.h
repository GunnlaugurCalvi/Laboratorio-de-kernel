/*
 * You must populate this struct with information from the kernel's task_struct.
 */
struct pid_info {
        char comm[16];
        pid_t pid;
        long state;
};

/*
 * This struct is used to communicate with the kernel in part II.
 */
struct kernellab_message {
	pid_t pid;
	struct pid_info *address;
};

/*
 * This enum is used to send commands to ioctl.
 */
enum KernellabCommand {
	RESET
};
