#include "gpiolib.h"

#include <os/list.h>
#include <os/kmalloc.h>
#include <os/gpio/driver.h>
#include <os/gpio/consumer.h>
#include <os/err.h>
#include <os/string.h>
#include <os/of.h>
#include <os/spinlock.h>

struct list_head gpio_chips = LIST_HEAD_INIT(gpio_chips);

/* 初始化gpiochip所有desc，并将chip加入全局链表 */
int gpiochip_add(struct gpio_chip *chip) {
    chip->desc = kzalloc(sizeof(struct gpio_desc) * chip->ngpio);

    for (int i = 0; i < chip->ngpio; i++) {
        chip->desc[i].chip = chip;
        chip->desc[i].offset = i;
    }

    list_add(&gpio_chips, &chip->list);

    return 0;
}

void gpiochip_remove(struct gpio_chip *chip) {
    list_del(&chip->list);
    kfree(chip->desc);
}

struct gpio_chip *of_find_gpiochip_by_node(struct device_node *np) {
    struct gpio_chip *chip;
    list_for_each_entry(chip, &gpio_chips, struct gpio_chip, list) {
        if (chip->dev->of_node == np)
            return chip;
    }
    return NULL;
}

static int of_get_gpio_cells(struct device_node *np) {
    struct device_node *current = np;
    struct device_prop *prop = NULL;

    while (current) {
        prop = of_get_property_by_name(current, "#gpio-cells");
        if (!prop) {
            current = current->parent;
        } else {
            return be32_to_cpu(*(__be32 *)prop->value);
        }
    }
    return 2; // 默认是2
}

static struct gpio_chip* __of_gpio_phrase_one(const struct device_node *np, int index, const char *prop_name, struct of_phandle_args *out) {
    u32 prop_len = 0;
    u32 offset = 0;
    u32 current_index = 0;
    void* cells = of_get_property(np, prop_name, &prop_len);
    if (!cells || prop_len < 2 * sizeof(u32))
        return ERR_PTR(-ENOENT);
    
    while (offset < prop_len / sizeof(u32)) {
        u32 gpio_phandle = be32_to_cpu(*(u32 *)cells);
        struct device_node *gpio_controller_node = of_find_node_by_phandle(gpio_phandle);
        if (!gpio_controller_node) {
            return ERR_PTR(-ENODEV);
        }

        u32 cell_size = of_get_gpio_cells(gpio_controller_node);
        if (offset + 1 + cell_size > prop_len / sizeof(u32)) {
            return ERR_PTR(-EINVAL);
        }

        if (current_index == index) {
            struct gpio_chip *chip = of_find_gpiochip_by_node(gpio_controller_node);
            if (!chip) {
                return ERR_PTR(-ENODEV);
            }

            void *gpios = (void*)cells + sizeof(u32) + offset * sizeof(u32); // 跳过phandle和前面已经解析的cell

            for (int i = 0; i < cell_size; i++) {
                out->args[i] = be32_to_cpu(*((u32 *)gpios + i));
            }

            out->np = gpio_controller_node;
            out->args_count = cell_size;

            return chip;
        }

        offset += (1+cell_size); 
        current_index ++;
    }

    return ERR_PTR(-ENOENT);
}

/* 从设备树获取gpio */
struct gpio_desc *of_gpiod_get(struct device *dev, const char *con_id) {
    char prop_name[32];
    struct device_node* np = dev->of_node;

    if (!con_id) {
        memcpy(prop_name, "gpios", sizeof("gpios"));
    } else {
        size_t len = strlen(con_id);
        if (len == 0 || len > 25)
            return ERR_PTR(-EINVAL);
    
        snprintk(prop_name, len + 7, "%s-gpios", con_id);
    }

    struct of_phandle_args args;
    struct gpio_chip *chip = __of_gpio_phrase_one(np, 0, prop_name, &args);
    if (IS_ERR(chip)) {
        return ERR_CAST(chip);
    }
    

    u32 gpio_offset;
    u32 flags;

    int ret = chip->of_xlate(chip, &args, &gpio_offset, &flags);

    if (ret < 0) {
        return ERR_PTR(ret);
    }

    struct gpio_desc *desc = &chip->desc[gpio_offset];

    if (flags & GPIO_ACTIVE_LOW) {
        desc->flags |= GPIO_ACTIVE_LOW;
    } else {
        desc->flags &= ~GPIO_ACTIVE_LOW;
    }

    // dprintk("chip %s, offset %d\n",desc->chip->dev->name,desc->offset);

    return desc;
}

int gpiod_request(struct gpio_desc *desc, const char *con_id) {
    if (desc->requested) {
        return -EBUSY;
    }

    spinlock_t *lock = &desc->chip->lock;

    int flags = spin_lock_irqsave(lock);
    desc->requested = 1;
    desc->label = con_id;

    spin_unlock_irqrestore(lock, flags);

    return 0;
}

void gpiod_free(struct gpio_desc *desc) {
    if (!desc->requested) {
        return;
    }

    spinlock_t *lock = &desc->chip->lock;

    int flags = spin_lock_irqsave(lock);
    desc->requested = 0;
    desc->label = NULL;
    spin_unlock_irqrestore(lock, flags);
}

struct gpio_desc *gpiod_get(struct device *dev, const char *con_id, enum gpiod_flags flags) {
    struct gpio_desc *desc = of_gpiod_get(dev, con_id);
    if (IS_ERR(desc)) {
        return desc;
    }

    int err = gpiod_request(desc, con_id);
    if (err < 0) {
        goto err;
    }

    switch (flags) {
        case GPIOD_IN: 
            err = gpiod_direction_input(desc); break;
        case GPIOD_OUT_LOW: 
            err = gpiod_direction_output(desc, 0); break;
        case GPIOD_OUT_HIGH: 
            err = gpiod_direction_output(desc, 1); break;
        case GPIOD_ASIS: 
            break;
        default:  
            err = -EINVAL; break;
    }
    
    if ( err < 0) {
        gpiod_free(desc);
        goto err;
    }

    return desc;

err:
    return ERR_PTR(err);
}

void gpiod_put(struct gpio_desc *desc) {
    if (desc->requested) {
        gpiod_free(desc);
    }
}

static int gpiod_to_raw_value(struct gpio_desc *desc, int value) {
    if (desc->flags & GPIO_ACTIVE_LOW) {
        value = !value;
    }
        
    return value;
}

static int gpiod_to_logical_value(struct gpio_desc *desc, int raw_value) {
    if (desc->flags & GPIO_ACTIVE_LOW)
        raw_value = !raw_value;
    return raw_value;
}


int gpiod_direction_input(struct gpio_desc *desc) {
    return desc->chip->direction_input(desc->chip, desc->offset);
}

int gpiod_direction_output(struct gpio_desc *desc, int value) {
    int raw_value = gpiod_to_raw_value(desc, value);
    return desc->chip->direction_output(desc->chip, desc->offset, raw_value);
}


int gpiod_get_value(struct gpio_desc *desc) {
    int logic_val =  desc->chip->get(desc->chip, desc->offset);
    return gpiod_to_logical_value(desc, logic_val);
}

void gpiod_set_value(struct gpio_desc *desc, int value) {
    int raw_val = gpiod_to_raw_value(desc, value);
    desc->chip->set(desc->chip, desc->offset, raw_val);
}

int gpiod_is_active_low(struct gpio_desc *desc) {
    return (desc->flags & GPIO_ACTIVE_LOW) != 0;
}

// void gpiod_set_consumer_name(struct gpio_desc *desc, const char *name) {
//     spinlock_t *lock = &desc->chip->lock;

//     int flags = spin_lock_irqsave(lock);
//     desc->label = name;
//     spin_unlock_irqrestore(lock, flags);
// }
