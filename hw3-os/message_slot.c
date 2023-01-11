#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/module.h>

#include "message_slot.h"

MODULE_LICENSE("GPL");

slot *slot_list[257];

static int device_open(struct inode *inode, struct file *file)
{

    int minor_number = iminor(inode);
    device_file *d_file;


    d_file = (device_file *)kmalloc(sizeof(device_file), GFP_KERNEL);

    if (d_file == NULL)
    {
        return FALIURE;
    }

    d_file->minor_number = minor_number;
    d_file->activeChannel = NULL;

    file->private_data = (void *)d_file;


    return SUCCSESS;
}

static int device_release(struct inode *inode, struct file *file)
{
    kfree(file->private_data);

    return SUCCSESS;
}

static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    int size, i;
    char *message;
    device_file *d_file;


    d_file = (device_file *)(file->private_data);

    if (buffer == NULL || d_file->activeChannel==NULL ||d_file->activeChannel->channel_id == 0)
    {
        return -EINVAL;
    }

    // get last message
    message = d_file->activeChannel->message;
    size = d_file->activeChannel->message_size;

    if (size == 0)
    {
        return -EWOULDBLOCK;
    }

    if (length < size)
    {
        return -ENOSPC;
    }

    // transfer to user's buffer
    for (i = 0; i < size; ++i)
    {
        if (put_user(message[i], buffer + i) != 0)
        {
            return -EIO;
        }
    }

    return i;
}

static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
    int i;
    char kbuffer[BUFFER];
    device_file *d_file;


    d_file = (device_file *)(file->private_data);

    if (buffer == NULL || d_file->activeChannel==NULL || (d_file->activeChannel->channel_id == 0))
    {
        return -EINVAL;
    }

    if (length == 0 || length > BUFFER)
    {
        return -EMSGSIZE;
    }

    // transfer to kernel buffer
    for (i = 0; i < length; ++i)
    {
        if (get_user(kbuffer[i], &buffer[i]) != 0)
        {
            return -EIO;
        }
    }

    // update file
    d_file->activeChannel->message_size = length;
    for (i = 0; i < length; ++i)
    {
        d_file->activeChannel->message[i] = kbuffer[i];
    }

    return i;
}

static long device_ioctl(struct file *file, unsigned int ioctl_command_id, unsigned long ioctl_param)
{
    device_file *d_file;
    channel *tmp;


    if (ioctl_command_id != MSG_SLOT_CHANNEL || ioctl_param == 0)
    {
        return -EINVAL;
    }

    d_file = (device_file *)(file->private_data);

    if (slot_list[d_file->minor_number] == NULL)
    {
        slot *newSlot = (slot *)kmalloc(sizeof(slot), GFP_KERNEL);
        channel *zero = (channel *)kmalloc(sizeof(channel), GFP_KERNEL);
        if (newSlot == NULL || zero == NULL)
        {
            return -ENOMEM;
        }
        zero->channel_id = 0;
        zero->message_size = 0;
        zero->next = NULL;
        newSlot->head = zero;
        slot_list[d_file->minor_number] = newSlot;
    }

    tmp = slot_list[d_file->minor_number]->head;
    while (tmp->next != NULL)
    {
        if (tmp->channel_id == ioctl_param)
        {
            d_file->activeChannel = tmp;
            return SUCCSESS;
        }
        tmp = tmp->next;
    }
    if (tmp->channel_id == ioctl_param)
    {
        d_file->activeChannel = tmp;
        return SUCCSESS;
    }

    // new channel
    tmp->next = kmalloc(sizeof(channel), GFP_KERNEL);
    if (tmp->next == NULL)
    {
        return -ENOMEM;
    }
    tmp->next->channel_id = ioctl_param;
    tmp->next->message_size = 0;
    tmp->next->next = NULL;
    d_file->activeChannel = tmp->next;


    return SUCCSESS;
}

struct file_operations Fops =
    {
        .owner = THIS_MODULE,
        .read = device_read,
        .write = device_write,
        .open = device_open,
        .unlocked_ioctl = device_ioctl,
        .compat_ioctl = device_ioctl,
        .release = device_release,
};

static int __init simple_init(void)
{
    int reg = -1;
    int i;


    reg = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &Fops);

    if (reg < 0)
    {
        printk(KERN_ERR "%s registraion failed for  %d\n", DEVICE_NAME, MAJOR_NUMBER);
        return reg;
    }

    for (i = 0; i < 257; i++)
    {
        slot_list[i] = NULL;
    }


    return SUCCSESS;
}

static void __exit simple_cleanup(void)
{
    channel *currentChannel;
    channel *tmp;
    int i;


    for (i = 0; i < 257; i++)
    {
        if (slot_list[i] != NULL)
        {
            currentChannel = slot_list[i]->head;

            while (currentChannel != NULL)
            {
                tmp = currentChannel->next;
                kfree(currentChannel);
                currentChannel=tmp;
            }
        }
    }

    unregister_chrdev(MAJOR_NUMBER, DEVICE_NAME);
}

module_init(simple_init);
module_exit(simple_cleanup);