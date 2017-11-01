/*
 *  chardev.c: Creates a read-only char device that says how many times
 *  you've read from the dev file
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>	/* for put_user */
#include <charDeviceDriver.h>

MODULE_LICENSE("GPL");

/* 
 * This function is called whenever a process tries to do an ioctl on our
 * device file. We get two extra parameters (additional to the inode and file
 * structures, which all device functions get): the number of the ioctl called
 * and the parameter given to the ioctl function.
 *
 * If the ioctl is write or read/write (meaning output is returned to the
 * calling process), the ioctl call returns the output of this function.
 *
 */

DEFINE_MUTEX  (devLock);

/* Current size of all the messages stored in the list */
static int  CURRENT_ALL_MESG_SIZE  = 0;

struct node {
	char *message;
	struct node *next;
	int size;
} ;

struct queue {
	struct node *front;
	struct node *end;
} ;

static void init(struct queue *q){
	q->front = NULL;
	q->end = NULL;
}

static void destroy(struct queue *q){
	struct node *head = q->front;
	struct node *temp;

	while(head){
		temp = head;
		head = head->next;
		kfree(temp->message);
		kfree(temp);
	}
}


static int enqueue (struct queue *q, char *message){
	struct node *n = kmalloc(sizeof(struct node), GFP_KERNEL);
	if(!n) return -ENOMEM;
	n->message = message;
	if(!q->front) {
		q->front = n;
		q->end = n;
	} else {
		q->end->next = n;
		q->end = q->end->next;
	}	
	printk(KERN_INFO "Message1: %s \n", q->front->message);
	return 0;
}


static char *dequeue (struct queue *q){
	struct node* n;
	char* message;
	if(!q->front) return NULL;
	n = q->front;
	message = n->message;
	q->front = q->front->next;
	printk(KERN_INFO "Message: %s \n", message);
	kfree(n);
	return message;
}




static struct queue message_queue;

/*
 * This function is called when the module is loaded
 */
int init_module(void)
{
    Major = register_chrdev(0, DEVICE_NAME, &fops);
	if (Major < 0) {
	  printk(KERN_ALERT "Registering char device failed with %d\n", Major);
	  return Major;
	}

	/* initialise the empty queue for message storage */
	init(&message_queue);

	// printk(KERN_INFO "I was assigned major number %d. To talk to\n", Major);
	// printk(KERN_INFO "the driver, create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
	// printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
	// printk(KERN_INFO "the device file.\n");
	// printk(KERN_INFO "Remove the device file and module when done.\n");

	return SUCCESS;
}


/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void)
{
	/* free all the allocated messages */
	destroy(&message_queue);

	/*  Unregister the device */
	unregister_chrdev(Major, DEVICE_NAME);
}


/* 
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
    
    mutex_lock (&devLock);
    if (Device_Open) {
		mutex_unlock (&devLock);
		return -EBUSY;
    }
    Device_Open++;
    mutex_unlock (&devLock);

    try_module_get(THIS_MODULE);
    
    return SUCCESS;
}

/* Called when a process closes the device file. */
static int device_release(struct inode *inode, struct file *file)
{
    mutex_lock (&devLock);
	Device_Open--;		/* We're now ready for our next caller */
	mutex_unlock (&devLock);
	
	/* 
	 * Decrement the usage count, or else once you opened the file, you'll
	 * never get get rid of the module. 
	 */
	module_put(THIS_MODULE);

	return 0;
}



/* 
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
			   char *buffer,	/* buffer to fill with data */
			   size_t length,	/* length of the buffer */
			   loff_t * offset)
{
	/*
	 * Number of bytes actually written to the buffer 
	 */
	int bytes_read = 0;

	/* result of function calls */
	int result;

	/* we retrieve the head of the queue */
	char *message = dequeue(&message_queue);

	if(!message) return -EAGAIN;
	printk(KERN_INFO "Retrieved the message: %s \n", message);
	/* 
	 * Actually put the data into the buffer 
	 */
	while (length && *message) {

		/* 
		 * The buffer is in the user data segment, not the kernel 
		 * segment so "*" assignment won't work.  We have to use 
		 * put_user which copies data from the kernel data segment to
		 * the user data segment. 
		 */
		result = put_user(*(message++), buffer++);
		if (result != 0) {
		         return -EFAULT;
		}
		    
		length--;
		bytes_read++;
	}

	kfree(message);

	CURRENT_ALL_MESG_SIZE -= bytes_read;
	/* 
	 * Most read functions return the number of bytes put into the buffer
	 */
	return bytes_read;
}



/* Called when a process writes to dev file: echo "hi" > /dev/hello  */
static ssize_t 
device_write(struct file *filp, const char *buffer, size_t length, loff_t * off)
{

	/*
	 * Number of bytes actually written to the buffer 
	 */
	int bytes_written = 0;

	/* result of function calls */
	int result;

	char *message, *msg_head;

	if(length > MAX_MESSAGE_SIZE) return -EINVAL;

	// TODO ask when to return this error, straight away like now
	// or during the reading
	if(length + CURRENT_ALL_MESG_SIZE  > MAX_ALL_MESG_SiZE)
		return -EAGAIN;


	/* allocate size for the message */
	message = kmalloc(length+1, GFP_KERNEL);
	if(!message) return -ENOMEM;
	message[length] = '\0';
	msg_head =  message;

	printk(KERN_INFO "Input: %s \n", buffer);

	/* 
	 * Actually put the data into the buffer 
	 */
	while (length && buffer) {

		/* 
		 * The buffer is in the user data segment, not the kernel 
		 * segment so "*" assignment won't work.  We have to use 
		 * put_user which copies data from the kernel data segment to
		 * the user data segment. 
		 */
		result = get_user(*(message++), buffer++);
		if (result != 0) {
		         return -EFAULT;
		}
		    
		length--;
		bytes_written++;
	}
	printk(KERN_INFO "Input: %s \n", msg_head);
	if(enqueue(&message_queue, msg_head) < 0) return -ENOMEM;

	CURRENT_ALL_MESG_SIZE += bytes_written;

	/* 
	 * Most read functions return the number of bytes put into the buffer
	 */
	return bytes_written;

}
