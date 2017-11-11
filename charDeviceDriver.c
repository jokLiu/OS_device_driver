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
//---
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/delay.h>
//---

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

/* Max size of all messages, 
   initially 2MB = 2 * 1024 * 1024 Bytes = 2097152 bytes */
static int MAX_ALL_MESG_SiZE = 2097152;

/* Current size of all the messages stored in the list */
static int  CURRENT_ALL_MESG_SIZE  = 0;

/* struct to hold a single message in the queue */
struct node {
	char *message;
	struct node *next;
    int size;
} ;

/* queue for the faster processing */
struct queue {
	struct node *front;
	struct node *end;
} ;

/* initializing the queue */
static void init(struct queue *q){
	q->front = NULL;
	q->end = NULL;
}

/* destroying the queue */
static void destroy(struct queue *q){
	struct node *head = q->front;
	struct node *temp;

	/* when the queue is destroyed during the 
	   device removal, we deallocate all the messages and nodes */
	while(head){
		temp = head;
		head = head->next;
		kfree(temp->message);
		kfree(temp);
	}
}

/* put a new message to the queue */
static int enqueue (struct queue *q, char *message, int length){
	/* we allocate the new node for storing a message */
	struct node *n = kmalloc(sizeof(struct node), GFP_KERNEL);

	/* if allocation failed, abort the current action */
	if(!n) return -ENOMEM;
	n->next=NULL;
	/* assign the message to the new node */
	n->message = message;

    /* add it's size */
    n->size = length;

	/* if the queue is empty we add our new node as
	   the front and end of the queue */
	if(!q->front) {
		q->front = n;
		q->end = n;
	} 
	/* if queue is not empty, we add it to the end
	   and make the end point to the new node */
	else {
		q->end->next = n;
		q->end = q->end->next;
	}	

	return 0;
}

/* retrieve message from the queue and remove it */
static char *dequeue (struct queue *q){
	struct node* n;
	char* message;

	/* if the queue is empty, we return NULL
	   indicating that reading is not possible */
	if(!q->front) return NULL;

	/* we get the head of the queue to return */
	n = q->front;

	/* we retrieve the message from that node */
	message = n->message;
    CURRENT_ALL_MESG_SIZE -= n->size;

	/* make queue point to the next element in the queue */
	q->front = q->front->next;

	/* we free a node to release the allocated memory */
	kfree(n);

	/* return the message */
	return message;
}



/* queue which keeps all the messages passed to the driver */
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

	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);

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
    try_module_get(THIS_MODULE);
    
    return SUCCESS;
}

/* Called when a process closes the device file. */
static int device_release(struct inode *inode, struct file *file)
{
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
	/* Number of bytes actually written to the buffer */
	int bytes_read = 0;

    printk(KERN_ALERT "length to read: %d\n", (int)length);

	/* result of function calls */
	int result;

	/* message to retrieve from the queue */
	char *message, *msg_head;

	/* lock the queue until we are reading from it */
	mutex_lock (&devLock);

	/* we retrieve the head of the queue */
	message = dequeue(&message_queue);

    printk(KERN_ALERT "current all msg size: %d\n", CURRENT_ALL_MESG_SIZE);
 
	/* unlock the queue after reading */
	mutex_unlock (&devLock);

    /* indicate that no message is available */
    if(!message) return -EAGAIN;

	// TODO remove all printings
//	printk(KERN_ALERT "Message read: %s\n", message);

	/*  Actually put the data into the buffer */
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

	/* we free the message because it is no longer needed */
	kfree(msg_head);

	/* Most read functions return the number of bytes put into the buffer */
	return bytes_read;
}



/* Called when a process writes to dev file: echo "hi" > /dev/hello  */
static ssize_t 
device_write(struct file *filp, const char *buffer, size_t length, loff_t * off)
{

	/* Number of bytes actually written from the buffer */
	int bytes_written = 0;

	/* result of function calls */
	int result;

	/* message to be returned to the user */
	char *message, *msg_head;

    // TODO make sure that this works
	/* if the message to be written is larger than
	   the maximum message size which is 4KB then return an error 
	   No need to lock it because MAX_MESSAGE_SIZE is never changed */
	if(length > MAX_MESSAGE_SIZE) return -EINVAL;

	/* allocate size for the message */
	message = kmalloc(length, GFP_KERNEL);

	/* if allocation failed, abort the current action */
	if(!message) return -ENOMEM;
	
	/* keep the head of the message to add it into the queue */
	msg_head =  message;

	/* Actually put the data into the buffer */
	while (length && *buffer) {

		/* 
		 * The buffer is in the user data segment, not the kernel 
		 * segment so "*" assignment won't work.  We have to use 
		 * get_user which copies data from the user data segment to
		 * the kernel data segment. 
		 */
		result = get_user(*(message++), buffer++);
		if (result != 0) {
		         return -EFAULT;
		}
		    
		length--;
		bytes_written++;
	}

	/* lock the queue until we are adding new message */
	mutex_lock (&devLock);

	// TODO make sure that this works
    /* if the message to be written is larger than the remaining size
     * of all messages, then we return an error */
	if(bytes_written + CURRENT_ALL_MESG_SIZE  > MAX_ALL_MESG_SiZE){
		/* unlock the queue after finishing the enqueueing */
		mutex_unlock (&devLock);

		/* message was discarded, so release the memory */
		kfree(msg_head);

		return -EAGAIN;
	}
	
	/* we add the message to the queue, 
	   if the enqueuing failed, it means there was not enough
	   memory left, so we abort, and return an error */
	if(enqueue(&message_queue, msg_head, bytes_written) < 0) {

		/* unlock the queue after finishing the enqueing */
		mutex_unlock (&devLock);

		/* message was discarded, so release the memory */
		kfree(msg_head);
		return -ENOMEM;
	}
    /* update the currently stored message size */
    CURRENT_ALL_MESG_SIZE += bytes_written;

    printk(KERN_ALERT "current all msg size: %d\n", CURRENT_ALL_MESG_SIZE);

	/* unlock the queue after finishing the enqueueing */
	mutex_unlock (&devLock);

	/* Most read functions return the number of bytes put into the buffer */
	return bytes_written;

}



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
static long 
device_ioctl(struct file *file, 
			unsigned int ioctl_num, 
			unsigned long ioctl_param)
{
	// TODO test if ioctl works
	/* we check whether the right flag is set */
	if(ioctl_num == SET_MAX_SIZE){
		
		 /* check if the maximum received is bigger than the old maximum, or
		    new maximum is bigger than the size of all messages currently held */
		if(ioctl_param > MAX_ALL_MESG_SiZE ||
		   ioctl_param > CURRENT_ALL_MESG_SIZE){

            /* lock the queue until we are updating the
               max all message size */
            mutex_lock (&devLock);

		   	/* set the new max size of all messages */
			MAX_ALL_MESG_SiZE = ioctl_param;

            /* unlock the queue after finishing the update */
            mutex_unlock (&devLock);

            // TODO remove printing
            printk(KERN_INFO "'size %d'.\n", MAX_ALL_MESG_SiZE);

			/* return success */
			return 0;
		}
	}
	return -EINVAL;
}

// TODO test ioctl and test blocking reads and writes
// TODO lock ioctl function