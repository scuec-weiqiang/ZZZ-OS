#include <os/init.h>

#include "memory_usage.h"

static int memory_after_tests_init(void)
{
    test_memory_usage_report("after_tests");
    return 0;
}

late_initcall(memory_after_tests_init);
