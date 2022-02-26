# reveng_rtkit
<p align="center">
    <img alt="Language" src="https://img.shields.io/badge/Language-C-green" />
    <img alt="Compiled with" src="https://img.shields.io/badge/Compiled%20with-gcc-blue" />
    <img alt="Tested on" src="https://img.shields.io/badge/Tested%20on-Ubuntu%2021.04%20(Hirsute%20Hippo)%20x86__64%20and%20Kernel%205.11.0--49--generic-yellow" />
</p>
<p align="center">
    <img alt="Category" src="https://img.shields.io/badge/Category-Post%20Exploitation%2F%20Persistence%2F%20Stealth-red" />
    <img alt="License" src="https://img.shields.io/badge/License-MIT-yellow.svg" />
</p>

***`reveng_rtkit`*** is a Linux Kernel mode (aka LKM) based rootkit targeting Linux Kernel: 5.11.0-49-generic as it was only tested on it till now. This project is heavily inspired by [Heroin](https://web.archive.org/web/20140701183221/https://www.thc.org/papers/LKM_HACKING.html#A-b) by  Runar Jensen (didn't get any of his social media handle ;( ) and [Diamorphine](https://github.com/m0nad/Diamorphine/) by [@m0nadlabs](https://twitter.com/m0nadlabs) open source LKM rootkit projects. Especially, the `Syscall interception mechanism section` was totally taken from [Diamorphine](https://github.com/m0nad/Diamorphine/) by [@m0nadlabs](https://twitter.com/m0nadlabs) repo.

### reveng_rtkit mechanism:

![](https://github.com/reveng007/reveng_rtkit/blob/main/reveng_rtkit_mechanism.jpeg?raw=true)

### <ins>Rootkit features</ins>:
| Sl. no. | Name | Features |
| ------- | ------- | -------- | 
| 1. | Finding Syscall Table address | By creating custom kallsyms_lookup_name function to get <ins>address</ins> of `sys_call_table` symbol from kernel memory. |
| 2. | Function Hooking | Get the <ins>address</ins> of the `syscall` from `sys_call_table` to get them hooked, then `modify CR0 register` to <ins>remove write protect bit</ins> and then modify/edit the `sys_call_table` and then again <ins>applying write protection to kernel memory</ins>. |
| 3. | Hide Rootkit | Hides itself by deleting itself(or entry) from responsible linked list. |
| 4. | Hide Processes/implants | Done in same way, as mentioned above in Sl. no. 2. |
| 5. | Unable to rmmod rootkit module | Using kernel function called `try_module_get()` makes impossible for admins to remove our rootkit. |
| 6. | Interactive Control | Implementing an IOCTL which manages the features of the rootkit and allows the user to send it commands. |
| 7. | Bypassing | Can bypass infamous [rkhunter](https://wiki.archlinux.org/title/Rkhunter) antirootkit |

### Let's see what functions will be called during loading the rootkit:

| Defined within Filename | Functions | function name in rootkit.c | Working | Effectivity of remove_rootkit() | Mode of access | 
| ----------------------- | --------- | -------------------------- | ------- | ----------------------------------------------------- | -- | 
| hide_show_helper.h | proc_lsmod_hide_rootkit() | hide_rootkit() | Hides rootkit from _"/proc/modules"_ file, _"/proc/kallsyms"_ file and "lsmod" command. | No effectivity | ./client_usermode | 
| hide_show_helper.h | sys_module_hide_rootkit() | hide_rootkit() | Hides rootkit from  "/sys/module/<THIS_MODULE>/" directory. | No effectivity | ./client_usermode | 
| hide_show_helper.h | proc_lsmod_show_rootkit() | show_rootkit() | Reveals our rootkit in _"/proc/modules"_ file, _"/proc/kallsyms"_ file and "lsmod" command. | Will work effectively | ./client_usermode | 
| hide_show_helper.h | sys_module_show_rootkit() | show_rootkit() | Reveals our rootkit in "/sys/module/<THIS_MODULE>/" directory. | _ | ./client_usermode |
| rootkit.c | tidy() | tidy() | In this function we do some clean up. If we don't do this, there will be some errors during unloading the rootkit using `rmmod`. | _ | _ | _
| rootkit.c | protect_rootkit() | protect_rootkit() | This is very simple function which just makes impossible to unload the rootkit by "rmmod rootkit" command even if it is visible. However it is still possible to unload by "rmmod -f rootkit" if kernel was compiled with support for forced unloading modules. &nbsp; link: [sysprog21.github.io](https://sysprog21.github.io/lkmpg/#building-modules-for-a-precompiled-kernel) | _ | ./client_usermode | 
| rootkit.c | remove_rootkit() | remove_rootkit() | Making rootkit removable from kernel using rmmod | _ | ./client_usermode |
| hook_syscall_helper.h | hacked_kill() | rootkit_init() and rootkit_exit(void) | Process/Implant Hiding | _ | cmd prompt: kill -31 \<pid> |
| hook_syscall_helper.h | hacked_kill() | rootkit_init() and rootkit_exit(void) | getting rootshell | _ | cmd prompt: kill -64 \<any pid> |

### NOTE:
> **Function tidy(), sys_module_hide_rootkit() and sys_module_show_rootkit() are not used in code. They were commented out. The reason behind that will be discussed in details in my blog post.**

### How to use it:
1. Clone the repo
```
$ git clone https://github.com/reveng007/reveng_rtkit.git
```
2. Enter the directory
```
$ cd reveng_rtkit/
```
3. Now, we have 2 directories: kernel_src and user_src.
- user_src:
Contains `usermode client code` to interact with our rootkit module (once it it loaded into the kernel) via the character device driver file.
- kernel_src:
Contains `kernelmode rootkit: reveng_rtkit` which will be responsible for the whole mayhem :wink:.

```
$ cd kernel_src/
$ make
$ sudo insmod reveng_rtkit.ko
```
![kernel_rootkit](https://user-images.githubusercontent.com/61424547/155671527-7bfb2487-e380-4812-b342-b0cb5c59cb11.png)

4. To interract with the kernel rootkit. Open another terminal
```
$ cd reveng_rtkit/user_src/
```
5. compile and run the code
```
$ gcc client_usermode.c -o client_usermode
$ sudo ./client_usermode
```
#### NOTE: Be sure to run the code with root priv., because we are interracting with device driver, which is a part of the Linux kernel.

![client_mode](https://user-images.githubusercontent.com/61424547/155754834-13bf9ee5-0bdd-4d30-af88-71a26a92dee8.png)

6. Another method to interract with it is via kill syscall interception:
- To hide process/implant:
```
$ kill -31 <pid>
```
![Screenshot from 2022-02-25 20-40-46](https://user-images.githubusercontent.com/61424547/155739121-b609d517-0b4f-4afc-a2db-b4ce7f331b17.png)

- To get root shell:
```
$ kill -64 <any pid>
```
![Screenshot from 2022-02-25 20-45-45](https://user-images.githubusercontent.com/61424547/155755082-d6ced40f-e0b0-47a3-8029-4f26b322df29.png)

#### NOTE:
>  This rootkit is capable of providing rootshell to only bash and sh shell, not others. Although, it is possible for other shells as well but with some tricks. We can use system() C function alike function in Linux Kernel programming, so that we 1st trigger a bash/sh shell then offer rootshell to the attacker. I  have'nt got that type of kernel function till now, but as soon as I get it, I will add it up. If anybody viewing this know about this, or interested to contribute, are most welcome to make a pull request.

- To remove this rootkit module: 1stly make module visible via `show` command using client_usermode file as reveng_rtkit while loading hides itself from being revealed (also change to `remove` mode, if you have made rootkit module to `protect` mode).
```
reveng007@ubuntoo ~/D/k/B/L/x/1/g/kernel_src> sudo ../user_src/client_usermode
[sudo] password for reveng007: 


[+] Created by @reveng007(Soumyanil)


|+++++++++++++++++++ Available commands ++++++++++++++++++|

hide		: Command to hide rootkit 
		=> In this mode, in no way this rootkit be removable

show		: Command to unhide rootkit 
		=> In this mode, rootkit_protect and rootkit_remove will work effectively

protect		: Command to make rootkit unremovable (even if it can be seen in usermode)

remove		: Command to make rootkit removable

kill -31 <pid>	: Command to hide/unhide running process. Applicable in normal shell prompt.
		=> write: `process` in the below prompt to close without any error

kill -64 <any pid>	: Command to get rootshell. Applicable in normal shell prompt.
		=> write: `root` in the below prompt to close without any error


[+] Driver file opened
[?] Enter the Value to send: show
[+] Written Value to Device file
[*] Reading Value from Device file: Value present in Device: show

[+] Device file closed
reveng007@ubuntoo ~/D/k/B/L/x/1/g/kernel_src> sudo ../user_src/client_usermode


[+] Created by @reveng007(Soumyanil)


|+++++++++++++++++++ Available commands ++++++++++++++++++|

hide		: Command to hide rootkit 
		=> In this mode, in no way this rootkit be removable

show		: Command to unhide rootkit 
		=> In this mode, rootkit_protect and rootkit_remove will work effectively

protect		: Command to make rootkit unremovable (even if it can be seen in usermode)

remove		: Command to make rootkit removable

kill -31 <pid>	: Command to hide/unhide running process. Applicable in normal shell prompt.
		=> write: `process` in the below prompt to close without any error

kill -64 <any pid>	: Command to get rootshell. Applicable in normal shell prompt.
		=> write: `root` in the below prompt to close without any error


[+] Driver file opened
[?] Enter the Value to send: remove
[+] Written Value to Device file
[*] Reading Value from Device file: Value present in Device: remove

[+] Device file closed
```
### Bypassing ***rkhunter*** antirootkit:

Here is the log file, that was generated: [file](https://github.com/reveng007/reveng_rtkit/blob/main/rkhunter.log)
- Two warnings are present:
1. Checking for suspicious (large) shared memory segments: For Notable markdown editor (This is not a threat lol!)
2. /usr/bin/lwp-request : [stackexchange](https://unix.stackexchange.com/questions/373718/rkhunter-gives-me-a-warning-for-usr-bin-lwp-request-what-should-i-do-debi)

So, these are not threats! cool!

### To-Do list :man_mechanic::
- Adding system() C function alike function in Linux Kernel programming, in order to open a new bash/sh prompt.
- Adding Linux Kernel Sockets.
- Surviving system reboot.
- Bypassing `chkrootkit antirootkit`.
    - Getting detected by `chkrootkit antirootkit` till now, under `chkproc section`: [chkproc.c](https://github.com/Magentron/chkrootkit/blob/master/chkproc.c)
&nbsp;
&nbsp;
![Screenshot from 2022-02-26 09-33-19](https://user-images.githubusercontent.com/61424547/155828253-812b8d7a-7326-4b57-9956-1fcaa92ec319.png)

### Detailed Blog article on `reveng_rtkit` LKM rootkit, is coming soon, where I will be explaining how I created this LKM rootkit step by step.

#### [Disclaimer]: Use this code for Educational/ Testing purpose only. Use it on those machines, on which you have legal permissions to perform testing. If somebody is found to use it for illegal jobs, author of the repo will not be held responsible for that.

### Resources:
1. https://github.com/pentesteracademy/linux-rootkits-red-blue-teams
2. Rootkit features: https://github.com/R3x/linux-rootkits
3. Simple LKM rootkit: https://theswissbay.ch/pdf/Whitepaper/Writing%20a%20simple%20rootkit%20for%20Linux%20-%20Ormi.pdf
4. IOCTL: https://github.com/Embetronicx/Tutorials/tree/master/Linux/Device_Driver/IOCTL
5. https://infosecwriteups.com/linux-kernel-module-rootkit-syscall-table-hijacking-8f1bc0bd099c
6. LKM HACKING: https://web.archive.org/web/20140701183221/https://www.thc.org/papers/LKM_HACKING.html
7. Hide Files: https://web.archive.org/web/20140701183221/https://www.thc.org/papers/LKM_HACKING.html#II.2.1.
8. Hide Processes: https://web.archive.org/web/20140701183221/https://www.thc.org/papers/LKM_HACKING.html#II.5.
9. Get Rootshell: https://xcellerator.github.io/posts/linux_rootkits_03/
10. https://sysprog21.github.io/lkmpg/
11. https://ish-ar.io/kprobes-in-a-nutshell/
12. file:///home/reveng007/Downloads/UNIX_and_Linux_based_Rootkits_Techniques_and_Count.pdf

### Author: @reveng007 (Soumyanil Biswas)
---
[![](https://img.shields.io/badge/Twitter-@reveng007-1DA1F2?style=flat-square&logo=twitter&logoColor=white)](https://twitter.com/reveng007)
[![](https://img.shields.io/badge/LinkedIn-@SoumyanilBiswas-0077B5?style=flat-square&logo=linkedin&logoColor=white)](https://www.linkedin.com/in/soumyanil-biswas/)
[![](https://img.shields.io/badge/Github-@reveng007-0077B5?style=flat-square&logo=github&logoColor=black)](https://github.com/reveng007/)

