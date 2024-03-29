
/* Global definition for the example character device driver */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long);

#define SUCCESS 0
/* Dev name as it appears in /proc/devices */
#define DEVICE_NAME "opsysmem"	

/* Max length of the message from the device, initially 4KB*/
#define MAX_MESSAGE_SIZE 4096   

/* ioctl call to reset the max size of all messages */
#define SET_MAX_SIZE 0


/* 
 * Global variables are declared as static, so are global within the file. 
 */
struct cdev *my_cdev;
       dev_t dev_num;

/* Major number assigned to the device driver */
static int Major;		

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.unlocked_ioctl = device_ioctl,
	.release = device_release
};
