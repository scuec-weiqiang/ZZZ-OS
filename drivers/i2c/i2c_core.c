
#include <os/i2c.h>
#include <os/bus.h>
#include <os/of.h>
#include <os/init.h>
#include <os/list.h>
#include <os/err.h>
#include <os/string.h>
#include <os/kmalloc.h>

//   platform_bus: device_register("i2c@10003000")
//   → 匹配到 platform_driver (i2c_controller_driver)
//     → probe() 被调用
//       → 初始化硬件（配置 GPIO、时钟、中断）
//       → 分配 i2c_adapter
//       → 设置 adapter->algo = &my_i2c_algorithm
//       → 调用 i2c_add_adapter(adapter)
//         → 扫描设备树子节点
//         → 为每个子节点创建 i2c_client
//         → client->dev.bus = &i2c_bus_type
//         → 调用 device_register(&client->dev)
//           → 匹配 i2c_bus_type 上的 i2c_driver
//           → 调用 i2c_driver->probe(client)

static struct device_type i2c_client_type = {
    .name = "i2c_client",
};

static struct device_type i2c_adapter_type = {
    .name = "i2c_adapter",
};
/**
 * i2c_verify_client - return parameter as i2c_client, or NULL
 * @dev: device, probably from some driver model iterator
 *
 * When traversing the driver model tree, perhaps using driver model
 * iterators like @device_for_each_child(), you can't assume very much
 * about the nodes you find.  Use this function to avoid oopses caused
 * by wrongly treating some non-I2C device as an i2c_client.
 */
struct i2c_client *i2c_verify_client(struct device *dev) {
	return (dev->type == &i2c_client_type)
			? to_i2c_client(dev)
			: NULL;
}

struct i2c_adapter *i2c_verify_adapter(struct device *dev) {
    return (dev->type == &i2c_adapter_type)
            ? to_i2c_adapter(dev)
            : NULL;
}

static const struct i2c_device_id *i2c_match_id(const struct i2c_device_id *id,
						const struct i2c_client *client)
{
	while (id->name[0]) {
		if (strcmp(client->name, id->name) == 0)
			return id;
		id++;
	}
	return NULL;
}

static int i2c_device_probe(struct device *dev) {
    struct i2c_client	*client = i2c_verify_client(dev);
	struct i2c_driver	*driver;

    if (!client) {
        return 0;
    }
    
    driver = to_i2c_driver(client->dev.driver);
    if (!driver) {
        return 0;
    }
    
    if (driver->probe) {
        return driver->probe(client, i2c_match_id(driver->id_table, client));
    }

    return 0;
}

static int i2c_device_remove(struct device *dev) {
    struct i2c_client	*client = i2c_verify_client(dev);
    struct i2c_driver	*driver;

    if (!client) {
        return 0;
    }

    driver = to_i2c_driver(client->dev.driver);

    if (driver && driver->remove) {
        return driver->remove(client);
    }

    return 0;
}

static int i2c_device_match(struct device *dev, const struct device_driver *drv) {
    // 根据设备树节点的 compatible 属性和驱动的 of_match_table 进行匹配
    if (!dev || !drv || !drv->of_match_table) {
        return -1;
    }
	return of_match_node(drv->of_match_table, dev->of_node) != NULL;
}

static struct bus_type i2c_bus_type = {
    .name = "i2c",
    .match = i2c_device_match,
    .probe = i2c_device_probe,
    .remove = i2c_device_remove,
};

int i2c_init() {
    return bus_register(&i2c_bus_type);
}
core_initcall(i2c_init);

static int i2c_adapter_nr = 0;
static int alloc_i2c_adapter_nr() {
    return i2c_adapter_nr++;
}

static struct i2c_client *i2c_client_alloc(const char *name, struct i2c_adapter *adapter) {
    struct i2c_client *client = kmalloc(sizeof(struct i2c_client));
    if (!client) {
        return NULL;
    }
    memset(client, 0, sizeof(struct i2c_client));
    strncpy(client->name, name, sizeof(client->name) - 1);
    client->adapter = adapter;
    client->dev.type = &i2c_client_type;
    client->dev.bus = &i2c_bus_type;
    return client;
}

static struct i2c_client *of_i2c_create_client(struct device_node *node, struct i2c_adapter *adapter) {
    struct i2c_client *client = i2c_client_alloc(node->name, adapter);
    if (!client) {
        return NULL;
    }
    INIT_LIST_HEAD(&client->dev.node);
    client->dev.of_node = node;
    client->dev.parent = &adapter->dev;
    client->dev.bus = &i2c_bus_type;
    client->dev.type = &i2c_client_type;
    client->dev.name = strdup(node->name);
    client->adapter = adapter;

    __be32 *addr = of_get_reg(node);
    if (addr) {
        client->addr = be32_to_cpu(*addr);
    } else {
        dprintk("i2c: warning: device node %s has no 'reg' property, using addr=0\n", node->name);
        client->addr = 0;
    }

    device_register(&client->dev);
    return client;
}

int i2c_add_adapter(struct i2c_adapter *adap) {
    if (!adap || !adap->algo) {
        return -EINVAL;
    }

    adap->dev.type = &i2c_adapter_type;
    adap->dev.bus = bus_get_by_name("i2c");
    adap->dev.name = adap->name;
    INIT_LIST_HEAD(&adap->dev.node);
    adap->nr = alloc_i2c_adapter_nr();

    device_register(&adap->dev);

    struct device_node *child;
    for_each_child_of_node(adap->dev.of_node, child) {
        of_i2c_create_client(child, adap);
    }
    
    return 0;
}

int i2c_del_adapter(struct i2c_adapter *adap) {
    if (!adap) {
        return -1;
    }
    return device_unregister(&adap->dev);
}

struct i2c_adapter* i2c_get_adapter(int nr) {
    struct device *dev;
    list_for_each_entry(dev, &i2c_bus_type.devices, struct device, node) {
        struct i2c_adapter *adap = to_i2c_adapter(dev);
        if (adap->nr == nr) {
            return adap;
        }
    }
    return ERR_PTR(ENODEV);
}

int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num) {
    if (!adap || !adap->algo || !adap->algo->master_xfer) {
        return -1;
    }
    return adap->algo->master_xfer(adap, msgs, num);
}

int i2c_driver_register(struct i2c_driver *driver) {
    if (!driver) {
        return -1;
    }
    driver->driver.bus = &i2c_bus_type;
    return driver_register(&driver->driver);
}


int i2c_driver_unregister(struct i2c_driver *driver) {
    if (!driver) {
        return -1;
    }
    driver_unregister(&driver->driver);
    return 0;
}

