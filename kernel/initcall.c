#include <mm/symbols.h>
#include <os/init.h>

void arch_initcalls_run(void)
{
    do_initcalls(archinitcall_start, archinitcall_end);
}

void core_initcalls_run(void)
{
    do_initcalls(initcall_start, initcall_end);
    do_initcalls(coreinitcall_start, coreinitcall_end);
}

void fs_initcalls_run(void)
{
    do_initcalls(fsinitcall_start, fsinitcall_end);
}

void device_initcalls_run(void)
{
    do_initcalls(deviceinitcall_start, deviceinitcall_end);
}

void late_initcalls_run(void)
{
    do_initcalls(lateinitcall_start, lateinitcall_end);
}
