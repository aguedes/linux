/*
 * User-space I/O driver support for HID subsystem
 * Copyright (c) 2012 David Herrmann
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/hid.h>
#include <linux/input.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/uhid.h>
#include <linux/wait.h>

#define UHID_NAME	"uhid"
#define UHID_BUFSIZE	32

struct uhid_device {
	struct mutex devlock;
	bool running;
	struct device *parent;

	__u8 *rd_data;
	uint rd_size;

	struct hid_device *hid;
	struct uhid_event input_buf;

	wait_queue_head_t waitq;
	spinlock_t qlock;
	struct uhid_event assemble;
	__u8 head;
	__u8 tail;
	struct uhid_event outq[UHID_BUFSIZE];
};

static void uhid_queue(struct uhid_device *uhid, const struct uhid_event *ev)
{
	__u8 newhead;

	newhead = (uhid->head + 1) % UHID_BUFSIZE;

	if (newhead != uhid->tail) {
		memcpy(&uhid->outq[uhid->head], ev, sizeof(struct uhid_event));
		uhid->head = newhead;
		wake_up_interruptible(&uhid->waitq);
	} else {
		pr_warn("Output queue is full\n");
	}
}

static int uhid_hid_start(struct hid_device *hid)
{
	struct uhid_device *uhid = hid->driver_data;
	unsigned long flags;

	spin_lock_irqsave(&uhid->qlock, flags);
	memset(&uhid->assemble, 0, sizeof(uhid->assemble));
	uhid->assemble.type = UHID_START;
	uhid_queue(uhid, &uhid->assemble);
	spin_unlock_irqrestore(&uhid->qlock, flags);

	return 0;
}

static void uhid_hid_stop(struct hid_device *hid)
{
	struct uhid_device *uhid = hid->driver_data;
	unsigned long flags;

	spin_lock_irqsave(&uhid->qlock, flags);
	memset(&uhid->assemble, 0, sizeof(uhid->assemble));
	uhid->assemble.type = UHID_STOP;
	uhid_queue(uhid, &uhid->assemble);
	spin_unlock_irqrestore(&uhid->qlock, flags);

	hid->claimed = 0;
}

static int uhid_hid_open(struct hid_device *hid)
{
	struct uhid_device *uhid = hid->driver_data;
	unsigned long flags;

	spin_lock_irqsave(&uhid->qlock, flags);
	memset(&uhid->assemble, 0, sizeof(uhid->assemble));
	uhid->assemble.type = UHID_OPEN;
	uhid_queue(uhid, &uhid->assemble);
	spin_unlock_irqrestore(&uhid->qlock, flags);

	return 0;
}

static void uhid_hid_close(struct hid_device *hid)
{
	struct uhid_device *uhid = hid->driver_data;
	unsigned long flags;

	spin_lock_irqsave(&uhid->qlock, flags);
	memset(&uhid->assemble, 0, sizeof(uhid->assemble));
	uhid->assemble.type = UHID_CLOSE;
	uhid_queue(uhid, &uhid->assemble);
	spin_unlock_irqrestore(&uhid->qlock, flags);
}

static int uhid_hid_power(struct hid_device *hid, int level)
{
	/* TODO: Handle PM-hints. This isn't mandatory so we simply return 0
	 * here.
	 */

	return 0;
}

static int uhid_hid_input(struct input_dev *input, unsigned int type,
				unsigned int code, int value)
{
	struct hid_device *hid = input_get_drvdata(input);
	struct uhid_device *uhid = hid->driver_data;
	unsigned long flags;

	spin_lock_irqsave(&uhid->qlock, flags);
	memset(&uhid->assemble, 0, sizeof(uhid->assemble));

	uhid->assemble.type = UHID_OUTPUT_EV;
	uhid->assemble.u.output_ev.type = type;
	uhid->assemble.u.output_ev.code = code;
	uhid->assemble.u.output_ev.value = value;

	uhid_queue(uhid, &uhid->assemble);
	spin_unlock_irqrestore(&uhid->qlock, flags);

	return 0;
}

static int uhid_hid_parse(struct hid_device *hid)
{
	struct uhid_device *uhid = hid->driver_data;

	return hid_parse_report(hid, uhid->rd_data, uhid->rd_size);
}

static int uhid_hid_get_raw(struct hid_device *hid, unsigned char rnum,
				__u8 *buf, size_t count, unsigned char rtype)
{
	/* TODO: we currently do not support this request. If we want this we
	 * would need some kind of stream-locking but it isn't needed by the
	 * main drivers, anyway.
	 */

	return -EOPNOTSUPP;
}

static int uhid_hid_output_raw(struct hid_device *hid, __u8 *buf, size_t count,
				unsigned char report_type)
{
	struct uhid_device *uhid = hid->driver_data;
	__u8 rtype;
	unsigned long flags;

	switch (report_type) {
		case HID_FEATURE_REPORT:
			rtype = UHID_FEATURE_REPORT;
			break;
		case HID_OUTPUT_REPORT:
			rtype = UHID_OUTPUT_REPORT;
			break;
		default:
			return -EINVAL;
	}

	if (count < 1 || count > UHID_DATA_MAX)
		return -EINVAL;

	spin_lock_irqsave(&uhid->qlock, flags);
	memset(&uhid->assemble, 0, sizeof(uhid->assemble));

	uhid->assemble.type = UHID_OUTPUT;
	uhid->assemble.u.output.size = count;
	uhid->assemble.u.output.rtype = rtype;
	memcpy(uhid->assemble.u.output.data, buf, count);

	uhid_queue(uhid, &uhid->assemble);
	spin_unlock_irqrestore(&uhid->qlock, flags);

	return count;
}

static struct hid_ll_driver uhid_hid_driver = {
	.start = uhid_hid_start,
	.stop = uhid_hid_stop,
	.open = uhid_hid_open,
	.close = uhid_hid_close,
	.power = uhid_hid_power,
	.hidinput_input_event = uhid_hid_input,
	.parse = uhid_hid_parse,
};

static int uhid_dev_create(struct uhid_device *uhid,
				const struct uhid_event *ev)
{
	struct hid_device *hid;
	int ret;

	ret = mutex_lock_interruptible(&uhid->devlock);
	if (ret)
		return ret;

	if (uhid->running) {
		ret = -EALREADY;
		goto unlock;
	}

	uhid->rd_size = ev->u.create.rd_size;
	uhid->rd_data = kzalloc(uhid->rd_size, GFP_KERNEL);
	if (!uhid->rd_data) {
		ret = -ENOMEM;
		goto unlock;
	}

	if (copy_from_user(uhid->rd_data, ev->u.create.rd_data,
				uhid->rd_size)) {
		ret = -EFAULT;
		goto err_free;
	}

	hid = hid_allocate_device();
	if (IS_ERR(hid)) {
		ret = PTR_ERR(hid);
		goto err_free;
	}

	strncpy(hid->name, ev->u.create.name, 128);
	hid->name[127] = 0;
	hid->ll_driver = &uhid_hid_driver;
	hid->hid_get_raw_report = uhid_hid_get_raw;
	hid->hid_output_raw_report = uhid_hid_output_raw;
	hid->bus = ev->u.create.bus;
	hid->vendor = ev->u.create.vendor;
	hid->product = ev->u.create.product;
	hid->version = ev->u.create.version;
	hid->country = ev->u.create.country;
	hid->phys[0] = 0;
	hid->uniq[0] = 0;
	hid->driver_data = uhid;
	hid->dev.parent = uhid->parent;

	uhid->hid = hid;
	uhid->running = true;

	ret = hid_add_device(hid);
	if (ret) {
		pr_err("Cannot register HID device\n");
		goto err_hid;
	}

	mutex_unlock(&uhid->devlock);

	return 0;

err_hid:
	hid_destroy_device(hid);
	uhid->hid = NULL;
	uhid->running = false;
err_free:
	kfree(uhid->rd_data);
unlock:
	mutex_unlock(&uhid->devlock);
	return ret;
}

static int uhid_dev_destroy(struct uhid_device *uhid)
{
	int ret;

	ret = mutex_lock_interruptible(&uhid->devlock);
	if (ret)
		return ret;

	if (!uhid->running) {
		ret = -EINVAL;
		goto unlock;
	}

	hid_destroy_device(uhid->hid);
	kfree(uhid->rd_data);
	uhid->running = false;

unlock:
	mutex_unlock(&uhid->devlock);
	return ret;
}

static int uhid_dev_input(struct uhid_device *uhid, struct uhid_event *ev)
{
	int ret;

	ret = mutex_lock_interruptible(&uhid->devlock);
	if (ret)
		return ret;

	if (!uhid->running) {
		ret = -EINVAL;
		goto unlock;
	}

	hid_input_report(uhid->hid, HID_INPUT_REPORT, ev->u.input.data,
				ev->u.input.size, 0);

unlock:
	mutex_unlock(&uhid->devlock);
	return ret;
}

static int uhid_char_open(struct inode *inode, struct file *file)
{
	struct uhid_device *uhid;

	uhid = kzalloc(sizeof(*uhid), GFP_KERNEL);
	if (!uhid)
		return -ENOMEM;

	mutex_init(&uhid->devlock);
	spin_lock_init(&uhid->qlock);
	init_waitqueue_head(&uhid->waitq);
	uhid->running = false;
	uhid->parent = NULL;

	file->private_data = uhid;
	nonseekable_open(inode, file);

	return 0;
}

static int uhid_char_release(struct inode *inode, struct file *file)
{
	struct uhid_device *uhid = file->private_data;

	uhid_dev_destroy(uhid);
	kfree(uhid);

	return 0;
}

static ssize_t uhid_char_read(struct file *file, char __user *buffer,
				size_t count, loff_t *ppos)
{
	struct uhid_device *uhid = file->private_data;
	int ret;
	unsigned long flags;
	size_t len;

	/* they need at least the "type" member of uhid_event */
	if (count < sizeof(__u32))
		return -EINVAL;

try_again:
	if (file->f_flags & O_NONBLOCK) {
		if (uhid->head == uhid->tail)
			return -EAGAIN;
	} else {
		ret = wait_event_interruptible(uhid->waitq,
						uhid->head != uhid->tail);
		if (ret)
			return ret;
	}

	ret = mutex_lock_interruptible(&uhid->devlock);
	if (ret)
		return ret;

	if (uhid->head == uhid->tail) {
		mutex_unlock(&uhid->devlock);
		goto try_again;
	} else {
		len = min(count, sizeof(*uhid->outq));
		if (copy_to_user(buffer, &uhid->outq[uhid->tail], len)) {
			ret = -EFAULT;
		} else {
			spin_lock_irqsave(&uhid->qlock, flags);
			uhid->tail = (uhid->tail + 1) % UHID_BUFSIZE;
			spin_unlock_irqrestore(&uhid->qlock, flags);
		}
	}

	mutex_unlock(&uhid->devlock);
	return ret ? ret : len;
}

static ssize_t uhid_char_write(struct file *file, const char __user *buffer,
				size_t count, loff_t *ppos)
{
	struct uhid_device *uhid = file->private_data;
	int ret;
	size_t len;

	/* we need at least the "type" member of uhid_event */
	if (count < sizeof(__u32))
		return -EINVAL;

	memset(&uhid->input_buf, 0, sizeof(uhid->input_buf));
	len = min(count, sizeof(uhid->input_buf));
	if (copy_from_user(&uhid->input_buf, buffer, len))
		return -EFAULT;

	switch (uhid->input_buf.type) {
		case UHID_CREATE:
			ret = uhid_dev_create(uhid, &uhid->input_buf);
			break;
		case UHID_DESTROY:
			ret = uhid_dev_destroy(uhid);
			break;
		case UHID_INPUT:
			ret = uhid_dev_input(uhid, &uhid->input_buf);
			break;
		default:
			ret = -EOPNOTSUPP;
	}

	/* return "count" not "len" to not confuse the caller */
	return ret ? ret : count;
}

static unsigned int uhid_char_poll(struct file *file, poll_table *wait)
{
	struct uhid_device *uhid = file->private_data;

	poll_wait(file, &uhid->waitq, wait);

	if (uhid->head != uhid->tail)
		return POLLIN | POLLRDNORM;

	return 0;
}

static const struct file_operations uhid_fops = {
	.owner		= THIS_MODULE,
	.open		= uhid_char_open,
	.release	= uhid_char_release,
	.read		= uhid_char_read,
	.write		= uhid_char_write,
	.poll		= uhid_char_poll,
	.llseek		= no_llseek,
};

static struct miscdevice uhid_misc = {
	.fops		= &uhid_fops,
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= UHID_NAME,
};

static int __init uhid_init(void)
{
	return misc_register(&uhid_misc);
}

static void __exit uhid_exit(void)
{
	misc_deregister(&uhid_misc);
}

module_init(uhid_init);
module_exit(uhid_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Herrmann <dh.herrmann@gmail.com>");
MODULE_DESCRIPTION("User-space I/O driver support for HID subsystem");
