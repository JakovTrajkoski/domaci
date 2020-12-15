#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/device.h>
#define BUFF_SIZE 80
MODULE_LICENSE("Dual BSD/GPL");
DECLARE_WAIT_QUEUE_HEAD(readQ);
DECLARE_WAIT_QUEUE_HEAD(writeQ);

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;

int lifo[16];
int broj_upisanih_vrednosti = 0;
int endRead = 0;
int pozicija_upis = 0;
int pozicija_ispis = 0;

int lifo_open(struct inode *pinode, struct file *pfile);
int lifo_close(struct inode *pinode, struct file *pfile);
ssize_t lifo_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
ssize_t lifo_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);

struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = lifo_open,
	.read = lifo_read,
	.write = lifo_write,
	.release = lifo_close,
};


int lifo_open(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully opened lifo\n");
		return 0;
}

int lifo_close(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully closed lifo\n");
		return 0;
}

ssize_t lifo_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) 
{
	int ret;
	char buff[BUFF_SIZE];
	long int len = 0;
	if (endRead){
		endRead = 0;
		return 0;
	}

	if(wait_event_interruptible(readQ,(broj_upisanih_vrednosti > 0)))
		return -ERESTARTSYS;

	if(broj_upisanih_vrednosti > 0)
	{
		broj_upisanih_vrednosti --;
		len = scnprintf(buff, BUFF_SIZE, "%x ", lifo[pozicija_ispis]);
		pozicija_ispis = pozicija_ispis + 1;
		if(pozicija_ispis == 16) pozicija_ispis = 0;
		ret = copy_to_user(buffer, buff, len);
		if(ret)
			return -EFAULT;
		printk(KERN_INFO "Succesfully read\n");
	}
	else
	{
		printk(KERN_WARNING "Lifo is empty\n"); 
	}
	wake_up_interruptible(&writeQ);

	endRead = 1;
	return len;
}

ssize_t lifo_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset) 
{
	char buff[BUFF_SIZE];
	int value;
	int ret;
	char *clan, *pozicija;
	int stanje = 3;

	ret = copy_from_user(buff, buffer, length);
	if(ret)
		return -EFAULT;
	buff[length-1] = '\0';

	if(wait_event_interruptible(writeQ, (broj_upisanih_vrednosti < 16)))
		return -ERESTARTSYS;

	clan = buff;

	while(stanje){

	pozicija = strchr(clan,';');
	if(pozicija == NULL)stanje = 0;
	else *pozicija = '\0';
		
	if(broj_upisanih_vrednosti < 16)
	{
		ret = sscanf(clan,"%x",&value);
		if(ret==1)//one parameter parsed in sscanf
		{
			clan = pozicija + 1;
			printk(KERN_INFO "Succesfully wrote value %x", value); 
			lifo[pozicija_upis] = value; 
			pozicija_upis = pozicija_upis + 1;
			broj_upisanih_vrednosti = broj_upisanih_vrednosti +1;
			if(pozicija_upis == 16) pozicija_upis = 0;
		}
		else	
		{
			printk(KERN_WARNING "Wrong command format\n");
		}
	}
	else
	{
		printk(KERN_WARNING "Lifo is full\n"); 
	}

	}//while

	wake_up_interruptible(&readQ);

	return length;
}

static int __init lifo_init(void)
{
   int ret = 0;
	int i=0;

	//Initialize array
	for (i=0; i<10; i++)
		lifo[i] = 0;

   ret = alloc_chrdev_region(&my_dev_id, 0, 1, "fifo");
   if (ret){
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   printk(KERN_INFO "char device region allocated\n");

   my_class = class_create(THIS_MODULE, "fifo_class");
   if (my_class == NULL){
      printk(KERN_ERR "failed to create class\n");
      goto fail_0;
   }
   printk(KERN_INFO "class created\n");
   
   my_device = device_create(my_class, NULL, my_dev_id, NULL, "fifo");
   if (my_device == NULL){
      printk(KERN_ERR "failed to create device\n");
      goto fail_1;
   }
   printk(KERN_INFO "device created\n");

	my_cdev = cdev_alloc();	
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;
	ret = cdev_add(my_cdev, my_dev_id, 1);
	if (ret)
	{
      printk(KERN_ERR "failed to add cdev\n");
		goto fail_2;
	}
   printk(KERN_INFO "cdev added\n");
   printk(KERN_INFO "Hello world\n");

   return 0;

   fail_2:
      device_destroy(my_class, my_dev_id);
   fail_1:
      class_destroy(my_class);
   fail_0:
      unregister_chrdev_region(my_dev_id, 1);
   return -1;
}

static void __exit lifo_exit(void)
{
   cdev_del(my_cdev);
   device_destroy(my_class, my_dev_id);
   class_destroy(my_class);
   unregister_chrdev_region(my_dev_id,1);
   printk(KERN_INFO "Goodbye, cruel world\n");
}


module_init(lifo_init);
module_exit(lifo_exit);
