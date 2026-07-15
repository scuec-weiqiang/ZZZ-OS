#include <mm/symbols.h>
#include <os/init.h>

static void run_initcall_range(const char *level, phys_addr_t start,
                               phys_addr_t end)
{
    initcall_t *entry;
    unsigned int index = 0;

    printk("initcall: %s range [%lx, %lx)\n", level,
           (unsigned long)start, (unsigned long)end);

    for (entry = (initcall_t *)start; entry < (initcall_t *)end;
         entry++, index++) {
        initcall_t fn = *entry;
        int ret;

        if (!fn) {
            printk("initcall: %s[%u] is NULL\n", level, index);
            continue;
        }

        printk("initcall: %s[%u] enter %lx\n", level, index,
               (unsigned long)fn);
        ret = fn();
        printk("initcall: %s[%u] exit %lx ret=%d\n", level, index,
               (unsigned long)fn, ret);
    }
}

void arch_initcalls_run(void)
{
    run_initcall_range("arch", archinitcall_start, archinitcall_end);
}

void core_initcalls_run(void)
{
    run_initcall_range("default", initcall_start, initcall_end);
    run_initcall_range("core", coreinitcall_start, coreinitcall_end);
}

void subsys_initcalls_run(void)
{
    run_initcall_range("subsys", subsysinitcall_start, subsysinitcall_end);
}

void fs_initcalls_run(void)
{
    run_initcall_range("fs", fsinitcall_start, fsinitcall_end);
}

void device_initcalls_run(void)
{
    run_initcall_range("device", deviceinitcall_start, deviceinitcall_end);
}

void late_initcalls_run(void)
{
    run_initcall_range("late", lateinitcall_start, lateinitcall_end);
}
