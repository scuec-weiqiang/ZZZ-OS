#include <stdio.h>
#include <sys/ps.h>

#define PS_MAX_TASKS 64

extern int _ps(struct ps_info *buf, int max);

static const char *state_name(int status)
{
    switch (status) {
    case 0:
        return "RUN";
    case 1:
        return "SLEEP";
    case 2:
        return "ZOMBIE";
    default:
        return "?";
    }
}

int main(void)
{
    struct ps_info tasks[PS_MAX_TASKS];
    int n;

    n = _ps(tasks, PS_MAX_TASKS);
    if (n < 0) {
        perror("ps");
        return 1;
    }

    printf("PID  CPU  STATE   ONRQ  RESCH  TYPE\n");
    for (int i = 0; i < n; i++) {
        printf("%-4d %-4d %-7s %-5d %-6d %s\n",
               tasks[i].pid,
               tasks[i].cpu,
               state_name(tasks[i].status),
               tasks[i].on_rq,
               tasks[i].need_resched,
               (tasks[i].flags & PS_FLAG_KTHREAD) ? "kthread" : "user");
    }

    return 0;
}
