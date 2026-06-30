#include <os/of.h>

int of_get_cpu_num() {
    static int cached_cpu_num;
    struct device_node *cpus_node;

    if (cached_cpu_num > 0) {
        return cached_cpu_num;
    }

    cpus_node = of_find_node_by_path("/cpus");
    if (!cpus_node) {
        return -1;
    }

    cached_cpu_num = of_get_child_node_count(cpus_node);
    return cached_cpu_num;
}
