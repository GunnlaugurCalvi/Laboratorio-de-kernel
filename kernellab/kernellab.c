/*
 * Kernellab
 */
#include <linux/module.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* file related operations */
#include <linux/types.h>        /* size_t */
#include <linux/errno.h>        /* error codes */
#include <linux/sched.h>        /* task_struct */
#include <linux/uaccess.h>      /* copy_to_user copy_from_user */
#include <linux/semaphore.h>    /* semaphore support */
#include <linux/kobject.h>      /* kobject support */
#include <linux/string.h>       /* similar to string.h */
#include <linux/sysfs.h>        /* sysfs support */
#include <linux/cdev.h>         /* character device support */
#include <linux/device.h>       /* device and class information */

#include "pidinfo.h"
#include <linux/thread_info.h>	/* use current in struct ADDED*/


struct kernellab_dev {
	int                     counter;  	/* number of times opened */
	struct semaphore        sem;            /* mutual exclusion semaphore */
	struct cdev             cdev;           /* Char device structure */
	int 			minor;		/* Kernellab1 or Kernellab2 */
	
};

struct kernellab_dev *kernellab_device;
static struct class *kl_class; /* Global variable for the device class */
static dev_t kl_dev; /* Global variable for the first device number */
static int nr_devs = 2; /* Number of devices */

static int current_count;
static int pid_count;
static int all_count;

/**
 * Sysfs operations
 */
static ssize_t kernellab_current_count(struct kobject *kobj,
				    struct kobj_attribute *attr, char *buf)
{ 
	return sprintf(buf, "%d\n", current_count); /* print count of current pids */
}

static struct kobj_attribute kernellab_current_count_attribute =
	__ATTR(current_count, 0440, kernellab_current_count, NULL);

static ssize_t kernellab_pid_count(struct kobject *kobj,
				    struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", pid_count); /* print count of pids */
}


static struct kobj_attribute kernellab_pid_count_attribute =
	__ATTR(pid_count, 0440, kernellab_pid_count, NULL);

static ssize_t kernellab_all_count(struct kobject *kobj,
				    struct kobj_attribute *attr, char *buf)
{

	/* Sum up pid_count and current_count and put it in all_count variable. 
 	*  Put semaphores around it for saftey.	*/	
	down(&kernellab_device->sem);	
	if(strcmp(attr->attr.name, "current_count") == 0){
		all_count  += current_count;	
	}
	else{
		all_count  += pid_count;
		all_count += current_count;
	}
	up(&kernellab_device->sem);	
  	return sprintf(buf, "%d\n", all_count);
}

static struct kobj_attribute kernellab_all_count_attribute =
	__ATTR(all_count, 0440, kernellab_all_count, NULL);



/* Setup list of sysfs entries */
static struct attribute *attrs[] = {
	&kernellab_current_count_attribute.attr,
	&kernellab_pid_count_attribute.attr,
	&kernellab_all_count_attribute.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static struct kobject *kernellab_kobj;


/**
 * Device file operations
 */
static int kernellab_open(struct inode *inode, struct file *filp)
{
	struct kernellab_dev *dev; /* device information */
	dev = container_of(inode->i_cdev, struct kernellab_dev, cdev);
	filp->private_data = dev; /* for other methods */

	/* Kernel device 1 or 2 is opened*/
	pr_info("kernellab: open(%d)\n", dev->minor);
	
	/* Be careful to add 1 to the correct counter if it is
 	*  kernellab1 or kernellab2.
 	*  Semaphores around it for saftey. 	*/
	down(&dev->sem);	
	if(dev->minor == 1){ 
		
		current_count += 1;
	}
	else{
		pid_count += 1;
	}	
	up(&dev->sem);

	/* How to use the device semaphore */
	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	/* Critical section */
	up(&dev->sem);

	
	return 0;
}

static int kernellab_release(struct inode *inode, struct file *filp)
{
	struct kernellab_dev *dev = filp->private_data;
	/* Kernel device 1 or 2 is closed */	
	pr_info("kernellab: close(%d)\n", dev->minor);
	
	return 0;
}

static long kernellab_ioctl(struct file *filp, unsigned int cmd,
			    unsigned long arg)
{
	struct kernellab_dev *dev = filp->private_data;
	/* Kernel device 1 or 2 is reset */
	pr_info("kernellab: ioctl(%d)\n", dev->minor);

	/* Wanna catch when RESET is called in user space 
 	*  and put the counters back to square 1 (square 0).
 	*  Semaphores around it for safety*/	
	down(&dev->sem);	
	if(dev->minor == 1 && cmd == RESET){
		current_count = 0;
	}
	else{
		pid_count = 0;
	}

	all_count = 0;
	
	up(&dev->sem);
	return -ENOIOCTLCMD;
}

static ssize_t kernellab_read(struct file *filp, char __user *buf, size_t count,
			   loff_t *f_pos)
{
	struct kernellab_dev *dev = filp->private_data;

	int errno;
	/* Kernel device 1 or 2 is red */
	pr_info("kernellab: read(%d)", dev->minor);	
	
	/* copy_to_user to get the current pid in user space. 
 	* Put semaphores around it for safety.	*/
	down(&dev->sem);		

	if(copy_to_user(buf, &current->pid, sizeof(count))){
		errno = -EFAULT;
	}

	up(&dev->sem);

	return errno;
}

static ssize_t kernellab_write(struct file *filp, const char __user *buf,
			       size_t count, loff_t *f_pos)
{
	struct kernellab_dev *dev = filp->private_data;

	int errno;
	
	struct task_struct *task;

	struct kernellab_message message;

	struct pid_info p_info;
	
	/* Kernel device 1 or 2 is writed*/
	pr_info("kernellab: write(%d)\n", dev->minor);

	/* Iff it is kernel device number 2
 	*  copy_from_user to get the sysfs struct message to kernel space
 	*  if the task pid is the same as message pid we got our pid 
 	*  and we fill in the p_info struct and use copy_to_user to
 	*  send it back to user space.
 	*  Put semaphores around it for safety */
	down(&dev->sem);
	
	if(dev->minor == 2){
	
		if(copy_from_user(&message, buf, count)){
			errno = -EFAULT;
		}
		if(copy_from_user(&p_info, message.address, sizeof(p_info))){
			errno = -EFAULT;
		}
	
		for_each_process(task){
			if(task->pid == message.pid){
				p_info.pid = task->pid;
				strcpy(p_info.comm, task->comm);
				p_info.state = task->state;
			}
		}
	
		if(copy_to_user(message.address, &p_info, sizeof(p_info))){
			errno = -EFAULT;
		}
	}

	up(&dev->sem);
	
	return errno;
}

static struct file_operations kernellab_fops = {
	.read           = kernellab_read,
	.write          = kernellab_write,
	.open           = kernellab_open,
	.release        = kernellab_release,
	.unlocked_ioctl = kernellab_ioctl,
	.owner          = THIS_MODULE
};

static int __init setup_devices(void)
{
	int err;
	struct device *dev_ret;
	dev_t dev;
	
	for (int i = 0; i < nr_devs; i++) {
		dev = MKDEV(MAJOR(kl_dev), MINOR(kl_dev) + i);
		dev_ret = device_create(kl_class, NULL, dev, NULL,
					"kernellab%d", i + 1);
		if (IS_ERR(dev_ret)) {
			err = PTR_ERR(dev_ret);
			while (i--) {
				dev = MKDEV(MAJOR(kl_dev), MINOR(kl_dev) + i);
				device_destroy(kl_class, dev);
			}
			return err;
		}
	}
	for (int i = 0; i < nr_devs; i++) {
		dev = MKDEV(MAJOR(kl_dev), MINOR(kl_dev) + i);
		sema_init(&kernellab_device[i].sem, 1);
		cdev_init(&kernellab_device[i].cdev, &kernellab_fops);
		kernellab_device[i].minor = i + 1; 	/* Initlize minor */
		if ((err = cdev_add(&kernellab_device[i].cdev, dev, 1)) < 0)
			return err;
	}
	return 0;
}

static int __init kernellab_init(void)
{
	int err;
	
	if ((err = alloc_chrdev_region(&kl_dev, 0, nr_devs, "kernellab")) < 0)
		goto out1;
	if (IS_ERR(kl_class = class_create(THIS_MODULE, "sty16"))) {
		err = PTR_ERR(kl_class);
		goto out2;
	}
	if (!(kernellab_device = kmalloc(nr_devs * sizeof(struct kernellab_dev),
					 GFP_KERNEL))) {
		err = -ENOMEM;
		goto out2;
	}
	memset(kernellab_device, 0, nr_devs * sizeof(struct kernellab_dev));
	if ((err = setup_devices()) < 0)
		goto out3;
	
		
	pr_info("kernellab: module INJECTED");
		
			
	kernellab_kobj = kobject_create_and_add("kernellab", kernel_kobj);
	if (!kernellab_kobj) {
		err = -ENOMEM;
		goto out4;
	}
	if ((err = sysfs_create_group(kernellab_kobj, &attr_group)))
		goto out4;
	return 0;
out4:
	for (int i = 0; i < nr_devs; i++) {
		dev_t dev = MKDEV(MAJOR(kl_dev), MINOR(kl_dev) + i);
		device_destroy(kl_class, dev);
	}
out3:
	class_destroy(kl_class);
out2:
	unregister_chrdev_region(kl_dev, nr_devs);
out1:
	return err;
}

static void __exit kernellab_exit(void)

{
	for (int i = 0; i < nr_devs; i++) {
		dev_t dev = MKDEV(MAJOR(kl_dev), MINOR(kl_dev) + i);
		cdev_del(&kernellab_device[i].cdev);
		device_destroy(kl_class, dev);
	}
	class_destroy(kl_class);
	unregister_chrdev_region(kl_dev, nr_devs);
	kobject_put(kernellab_kobj);

	
	pr_info("kernellab: module UNLOADED");
	
	kfree(kernellab_device);
}

module_init(kernellab_init);
module_exit(kernellab_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gunnlaugur Kristinn Hreidarsson <Gunnlaugur15@ru.is");
MODULE_AUTHOR("Hjalmar Orn Hannesson <Hjalmarh14@ru.is");
