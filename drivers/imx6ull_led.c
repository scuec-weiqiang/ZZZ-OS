#include <os/module.h>
#include <fs/fs.h>
#include <fs/cdev.h>
#include <os/gpio/consumer.h>
#include <os/platform_device.h>
#include <os/uaccess.h>
#include <os/err.h>
#include <os/string.h>
#include <os/kmalloc.h>

struct led_info {
    struct gpio_desc *gpio;
    struct cdev cdev;
    dev_t dev_num;
    struct device *dev;
};

struct led_info *led = NULL;

int led_open(struct inode *inode, struct file *file) {
    file->private_data = led;
    return 0;
}

int led_release(struct inode *inode, struct file *file) {
    file->private_data = NULL;
    return 0;
}


ssize_t led_write (struct file *file, const char __user *buf, size_t size, loff_t *offset) {
    char kbuf[16]; // 内核缓冲区，存储从用户空间复制的数据
    struct led_info *led = file->private_data;
    if (size > sizeof(kbuf) - 1)
        return -EINVAL;

    if (copy_from_user(kbuf, buf, size)) {
        return -EFAULT;
    }
    
    kbuf[size] = '\0'; // 确保字符串以'\0'结尾
    // dprintk("led_write: received data: size %d ,addr 0x%x, '%s'\n",size, (u32)buf, kbuf);

    if (strncmp(kbuf, "1\n",sizeof("1\n")) == 0) {
        gpiod_set_value(led->gpio, 1); // 点亮LED
    } else if (strncmp(kbuf, "0\n",sizeof("0\n")) == 0) {
        gpiod_set_value(led->gpio, 0); // 熄灭LED
    } else {
        return -EINVAL; // 无效的输入
    }

    return size;
}

static const struct file_operations led_fops = {
    .open = led_open,
    // .release = led_release,
    .write = led_write,
};

static int led_probe(struct platform_device *pdev) {
    int ret = 0;
    
    led = kzalloc(sizeof(struct led_info));
    if (!led) {
        dprintk("Failed to allocate memory for led_info\n");
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, led);

    led->gpio = gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);

    if (IS_ERR(led->gpio)) {
        dprintk("Get gpio failed!\n");
        ret = PTR_ERR(led->gpio);
        goto get_gpio_failed;
    } else {
        dprintk("Get gpio success!\n");
    }

    ret = alloc_chrdev_region(&led->dev_num,  1);
    if (ret) {
        dprintk("Alloc dev number failed!\n");
        goto alloc_devnr_failed;
    } else {
        dprintk("Device number is %d\n",led->dev_num);
    }
    
    ret = cdev_register("myledcdev", led->dev_num, &led_fops, led);
    if (ret) {
        dprintk("Add cdev failed!\n");
        goto cdev_failed;
    }

    dprintk("Create device success!\n");
    return 0;

cdev_failed:
alloc_devnr_failed:
    gpiod_put(led->gpio);
get_gpio_failed:
    kfree(led);
    return ret;
}

static int led_remove(struct platform_device *pdev)
{
    struct led_info *led = platform_get_drvdata(pdev);
    gpiod_set_value(led->gpio, 0); // 熄灭LED
    kfree(led);
    return 0;
}

static const struct of_device_id led_of_match[] = {
    { .compatible = "my,led", },
    {},
};

static  struct platform_driver led_driver = {
    .probe = led_probe,
    .remove = led_remove,
    .driver = {
        .name = "myled",
        .of_match_table = led_of_match
    }
};

module_platform_driver(led_driver);