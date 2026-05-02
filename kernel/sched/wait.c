#include <os/printk.h>
#include <os/kmalloc.h>
#include <os/wait.h>
#include <os/errno.h>
#include <os/sched.h>
#include <os/list.h>
#include <asm/ptrace.h>

int do_waitpid(pid_t pid, int *status) {
    while (1) {
        int found_child = 0;
        struct task_struct *child;
        list_for_each_entry(child, &current->children, struct task_struct, sibling) {
            if (pid  > 0) {
                if (child->pid !=  pid)
                    continue;
            } else if (pid == -1) {

            }
    
            found_child = 1;
            if (child->status == TASK_ZOMBIE) {
                dprintk("waitpid: found zombie child pid=%d, exit code=%d\n", child->pid, child->exit_code);
                // 取 exit code
                if (status) {
                    *status = child->exit_code;
                }

                pid_t pid = child->pid;
                    
                // 从 children 移除
                list_del(&child->sibling);

                task_detach_from_rq(child);

                // 释放资源
                task_destroy(child);

                return pid;
            }
        }

        // 没有任何子进程
        if (!found_child) {
            // 
            return -ECHILD;
        }
            
        // 有子进程但都没死 → 睡眠
        sleep_on(&current->wait_child);
    }
}

int do_wait(int *status) {
    return do_waitpid(-1, status);
}

int sys_waitpid(struct pt_regs *ctx) {
    pid_t pid = (pid_t)ctx->r[0];
    int *status = (int *)ctx->r[1];
    return do_waitpid(pid, status);
}