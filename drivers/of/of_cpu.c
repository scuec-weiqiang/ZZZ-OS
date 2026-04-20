#include <os/of.h>

int of_get_cpu_num() {
    struct device_node *cpus_node = of_find_node_by_path("/cpus");
    if (!cpus_node) {
        return -1;
    }
    return of_get_child_node_count(cpus_node);
}