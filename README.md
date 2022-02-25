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

***`reveng_rtkit`*** is a Linux Kernel mode (aka LKM) based rootkit targeting Linux Kernel: 5.11.0-49-generic as it was only tested on it till now.
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
| rootkit.c | tidy() | tidy() | In this function we do some clean up. If we don't do this, there will be some errors during unloading the rootkit using `rmmod`. | _ | _ | _
| rootkit.c | protect_rootkit() | protect_rootkit() | This is very simple function which just makes impossible to unload the rootkit by "rmmod rootkit" command even if it is visible. However it is still possible to unload by "rmmod -f rootkit" if kernel was compiled with support for forced unloading modules. &nbsp; link: [sysprog21.github.io](https://sysprog21.github.io/lkmpg/#building-modules-for-a-precompiled-kernel) | _ | ./client_usermode | 
| rootkit.c | remove_rootkit() | remove_rootkit() | Making rootkit removable from kernel using rmmod | _ | ./client_usermode |
| hook_syscall_helper.h | kill and getdents64 syscall | rootkit_init() and rootkit_exit(void) | Process/Implant Hiding | _ | cmd prompt: kill -31 \<pid> |
