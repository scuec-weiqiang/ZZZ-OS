#include <os/init.h>

#include "memory_usage.h"

static int memory_before_tests_init(void)
{
    test_memory_usage_report("before_tests");
    return 0;
}

late_initcall(memory_before_tests_init);
