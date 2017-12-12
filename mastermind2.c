/* YOUR FILE-HEADER COMMENT HERE */

/*
 * This file uses kernel-doc style comments, which is similar to
 * Javadoc and Doxygen-style comments.  See
 * ~/linux/Documentation/kernel-doc-nano-HOWTO.txt for details.
 */

/*
 * Getting compilation warnings?  The Linux kernel is written against
 * C89, which means:
 *  - No // comments, and
 *  - All variables must be declared at the top of functions.
 * Read ~/linux/Documentation/CodingStyle to ensure your project
 * compiles without warnings.
 */

#define pr_fmt(fmt) "mastermind2: " fmt

#include <linux/capability.h>
#include <linux/cred.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/uidgid.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>

#include "xt_cs421net.h"

#define NUM_PEGS 4
#define NUM_COLORS 6

/** true if user is in the middle of a game */
static bool game_active;

/** code that player is trying to guess */
static int target_code[NUM_PEGS];

/** tracks number of guesses user has made */
static unsigned num_guesses;

/** current status of the game */
static char game_status[80];

/** buffer that logs guesses for the current game */
static char *user_view;

static int uv_pos;

static DEFINE_SPINLOCK(lock);

/* Copy mm_read(), mm_write(), mm_mmap(), and mm_ctl_write(), along
 * with all of your global variables and helper functions here.
 */
/* YOUR CODE HERE */

/**
 * mm_read() - callback invoked when a process reads from
 * /dev/mm
 * @filp: process's file object that is reading from this device (ignored)
 * @ubuf: destination buffer to store output
 * @count: number of bytes in @ubuf
 * @ppos: file offset (in/out parameter)
 *
 * Write to @ubuf the current status of the game, offset by
 * @ppos. Copy the lesser of @count and (string length of @game_status
 * - *@ppos). Then increment the value pointed to by @ppos by the
 * number of bytes copied. If @ppos is greater than or equal to the
 * length of @game_status, then copy nothing.
 *
 * Return: number of bytes written to @ubuf, or negative on error
 */
static ssize_t mm_read(struct file *filp, char __user * ubuf, size_t count,
		       loff_t * ppos)
{
	spin_lock(&lock);
	int retval;
	count =
	    (count >
	     (sizeof(game_status) - *ppos)) ? (sizeof(game_status) -
					       *ppos) : count;
	retval = copy_to_user(ubuf, game_status+*ppos, count);
	if (retval < 0){
		spin_unlock(&lock);
		return -EINVAL;
	}
	*ppos += count;
	spin_unlock(&lock);
	return count;
	/* FIXME */
	return -EPERM;
}

/**
 * mm_write() - callback invoked when a process writes to /dev/mm
 * @filp: process's file object that is reading from this device (ignored)
 * @ubuf: source buffer from user
 * @count: number of bytes in @ubuf
 * @ppos: file offset (ignored)
 *
 * If the user is not currently playing a game, then return -EINVAL.
 *
 * If @count is less than NUM_PEGS, then return -EINVAL. Otherwise,
 * interpret the first NUM_PEGS characters in @ubuf as the user's
 * guess. For each guessed peg, calculate how many are in the correct
 * value and position, and how many are simply the correct value. Then
 * update @num_guesses, @game_status, and @user_view.
 *
 * <em>Caution: @ubuf is NOT a string; it is not necessarily
 * null-terminated.</em> You CANNOT use strcpy() or strlen() on it!
 *
 * Return: @count, or negative on error
 */
static ssize_t mm_write(struct file *filp, const char __user * ubuf,
			size_t count, loff_t * ppos)
{
	spin_lock(&lock);
	int retval;
	int guess[NUM_PEGS];
	int checked[NUM_PEGS];
	int i, j, bl_peg, wh_peg;
	char kernel_buff[80];
	if (!game_active)
		return -EPERM;
	retval = copy_from_user(kernel_buff, ubuf, count);
	if (retval < 0)
		return -EINVAL;
	for (i = 0; i < NUM_PEGS; i++)
		if (kernel_buff[i] >= '0' && kernel_buff[i] <= '5')
			guess[i] = (kernel_buff[i] - '0');
		else
			return -EPERM;
	for (i = 0; i < NUM_PEGS; i++)
		checked[i] = -1;

	bl_peg = 0;
	wh_peg = 0;
	for (i = 0; i < NUM_PEGS; i++)
		if (guess[i] == target_code[i])
			checked[i] = 1;
	for (i = 0; i < NUM_PEGS; i++) {
		if (checked[i] != 1)
			for (j = 0; j < NUM_PEGS; j++)
				if (checked[j] != 1
				    && guess[i] == target_code[j])
					checked[j] = 0;
	}
	for (i = 0; i < NUM_PEGS; i++)
		if (checked[i] == 1)
			bl_peg++;
		else if (checked[i] == 0)
			wh_peg++;

	num_guesses++;
	for (i = 0; i < sizeof(game_status); i++)
		game_status[i] = '\0';
	uv_pos +=
	    scnprintf(user_view + uv_pos, PAGE_SIZE - uv_pos, "%c%c%c%c%i%i",
		      kernel_buff[0], kernel_buff[1], kernel_buff[2],
		      kernel_buff[3], bl_peg, wh_peg);
	if(bl_peg<NUM_PEGS)
		scnprintf(game_status, sizeof(game_status),
		  	"Guess %u: %i black peg(s), %i white peg(s)\n", num_guesses,
		  	bl_peg, wh_peg);
	else{
		scnprintf(game_status, sizeof(game_status), "Correct! Game over\n");
		game_active = false;
		uv_pos+=scnprintf(user_view + uv_pos, PAGE_SIZE - uv_pos, "Winner");
	}
	/* FIXME */
	spin_unlock(&lock);
	return count;
}

/**
 * mm_mmap() - callback invoked when a process mmap()s to /dev/mm
 * @filp: process's file object that is mapping to this device (ignored)
 * @vma: virtual memory allocation object containing mmap() request
 *
 * Create a read-only mapping from kernel memory (specifically,
 * @user_view) into user space.
 *
 * Code based upon
 * <a href="http://bloggar.combitech.se/ldc/2015/01/21/mmap-memory-between-kernel-and-userspace/">http://bloggar.combitech.se/ldc/2015/01/21/mmap-memory-between-kernel-and-userspace/</a>
 *
 * You do not need to modify this function.
 *
 * Return: 0 on success, negative on error.
 */
static int mm_mmap(struct file *filp, struct vm_area_struct *vma)
{
	spin_lock(&lock);
	unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);
	unsigned long page = vmalloc_to_pfn(user_view);
	if (size > PAGE_SIZE)
		return -EIO;
	vma->vm_pgoff = 0;
	vma->vm_page_prot = PAGE_READONLY;
	if (remap_pfn_range(vma, vma->vm_start, page, size, vma->vm_page_prot)){
		spin_unlock(&lock);
		return -EAGAIN;
	}
	spin_unlock(&lock);
	return 0;
}

/**
 * mm_ctl_write() - callback invoked when a process writes to
 * /dev/mm_ctl
 * @filp: process's file object that is writing to this device (ignored)
 * @ubuf: source buffer from user
 * @count: number of bytes in @ubuf
 * @ppos: file offset (ignored)
 *
 * Copy the contents of @ubuf, up to the lesser of @count and 8 bytes,
 * to a temporary location. Then parse that character array as
 * following:
 *
 *  start - Start a new game. If a game was already in progress, restart it.
 *  quit  - Quit the current game. If no game was in progress, do nothing.
 *
 * If the input is none of the above, then return -EINVAL.
 *
 * <em>Caution: @ubuf is NOT a string; it is not necessarily
 * null-terminated.</em> You CANNOT use strcpy() or strlen() on it!
 *
 * Return: @count, or negative on error
 */
static ssize_t mm_ctl_write(struct file *filp, const char __user * ubuf,
			    size_t count, loff_t * ppos)
{
	spin_lock(&lock);
	int retval;
	char kernel_buff[80];
	int i;
	retval = copy_from_user(kernel_buff, ubuf, count);
	if ((count == 5) && (kernel_buff[0] == 's') && (kernel_buff[1] == 't')
	    && (kernel_buff[2] == 'a') && (kernel_buff[3] == 'r')
	    && (kernel_buff[4] == 't')) {
		target_code[0] = 0;
		target_code[1] = 0;
		target_code[2] = 1;
		target_code[3] = 2;
		num_guesses = 0;
		uv_pos = 0;
		for (i = 0; i < PAGE_SIZE; i++)
			user_view[i] = '\0';
		game_active = true;
		for (i = 0; i < sizeof(game_status); i++)
			game_status[i] = '\0';
		scnprintf(game_status, sizeof(game_status), "Starting game\n");
	} else if (count == 4 && kernel_buff[0] == 'q' && kernel_buff[1] == 'u'
		   && kernel_buff[2] == 'i' && kernel_buff[3] == 't') {
		game_active = false;
		for (i = 0; i < sizeof(game_status); i++)
			game_status[i] = '\0';
		scnprintf(game_status, sizeof(game_status),
			  "Game over. The code was     .\n");
		for (i = 0; i < 4; i++)
			game_status[24 + i] = (char)(target_code[i] + '0');
	} else{
		spin_unlock(&lock);
		return -EPERM;
	}
	spin_unlock(&lock);
	/* FIXME */
	return count;
	/* FIXME */
	return -EPERM;
}

static const struct file_operations mm_ctl_fops = {
	.write = mm_ctl_write,
};

static struct miscdevice mm_ctl_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mm_ctl",
	.fops = &mm_ctl_fops,
	.mode = 0666,
};

static const struct file_operations mm_fops = {
	.read = mm_read,
	.write = mm_write,
	.mmap = mm_mmap,
};

static struct miscdevice mm_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mm",
	.fops = &mm_fops,
	.mode = 0666,
};

/**
 * cs421net_top() - top-half of CS421Net ISR
 * @irq: IRQ that was invoked (ignored)
 * @cookie: Pointer to data that was passed into
 * request_threaded_irq() (ignored)
 *
 * If @irq is CS421NET_IRQ, then wake up the bottom-half. Otherwise,
 * return IRQ_NONE.
 */
static irqreturn_t cs421net_top(int irq, void *cookie)
{
	/* Part 4: YOUR CODE HERE */
	return 0;
}

/**
 * cs421net_bottom() - bottom-half to CS421Net ISR
 * @irq: IRQ that was invoked (ignore)
 * @cookie: Pointer that was passed into request_threaded_irq()
 * (ignored)
 *
 * Fetch the incoming packet, via cs421net_get_data(). If:
 *   1. The packet length is exactly equal to the number of digits in
 *      the target code, and
 *   2. If all characters in the packet are valid ASCII representation
 *      of valid digits in the code, then
 * Set the target code to the new code, and increment the number of
 * types the code was changed remotely. Otherwise, ignore the packet
 * and increment the number of invalid change attempts.
 *
 * During Part 5, update this function to change all codes for all
 * active games.
 *
 * Remember to add appropriate spin lock calls in this function.
 *
 * <em>Caution: The incoming payload is NOT a string; it is not
 * necessarily null-terminated.</em> You CANNOT use strcpy() or
 * strlen() on it!
 *
 * Return: always IRQ_HANDLED
 */
static irqreturn_t cs421net_bottom(int irq, void *cookie)
{
	/* Part 4: YOUR CODE HERE */
	return IRQ_HANDLED;
}

/**
 * mm_stats_show() - callback invoked when a process reads from
 * /sys/devices/platform/mastermind/stats
 *
 * @dev: device driver data for sysfs entry (ignored)
 * @attr: sysfs entry context (ignored)
 * @buf: destination to store game statistics
 *
 * Write to @buf, up to PAGE_SIZE characters, a human-readable message
 * containing these game statistics:
 *   - Number of pegs (digits in target code)
 *   - Number of colors (range of digits in target code)
 *   - Number of valid network messages (see Part 4)
 *   - Number of invalid network messages (see Part 4)
 *   - Number of active players (see Part 5)
 * Note that @buf is a normal character buffer, not a __user
 * buffer. Use scnprintf() in this function.
 *
 * @return Number of bytes written to @buf, or negative on error.
 */
static ssize_t mm_stats_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	/* Part 3: YOUR CODE HERE */
	return -EPERM;
}

static DEVICE_ATTR(stats, S_IRUGO, mm_stats_show, NULL);

/**
 * mastermind_probe() - callback invoked when this driver is probed
 * @pdev platform device driver data
 *
 * Return: 0 on successful probing, negative on error
 */
static int mastermind_probe(struct platform_device *pdev)
{
	/* Copy the contents of your original mastermind_init() here. */
	/* YOUR CODE HERE */
	int j, retval;
	
	pr_info("Initializing the game.\n");
	user_view = vmalloc(PAGE_SIZE);
	if (!user_view) {
		pr_err("Could not allocate memory\n");
		return -ENOMEM;
	}

	if (misc_register(&mm_dev))
		goto DEV_ERROR;
	if (misc_register(&mm_ctl_dev))
		goto CTL_DEV_ERROR;

	for (j = 0; j < sizeof(game_status); j++)
		game_status[j] = '\0';

	scnprintf(game_status, sizeof(game_status), "No game yet\n");

	game_active = false;

	/*
	 * You will need to integrate the following resource allocator
	 * into your code. That also means properly releasing the
	 * resource if the function fails.
	 */
	retval = device_create_file(&pdev->dev, &dev_attr_stats);
	if (retval) {
		pr_err("Could not create sysfs entry\n");
		pr_info("Freeing resources.\n");
		misc_deregister(&mm_dev);
		misc_deregister(&mm_ctl_dev);
		vfree(user_view);
	}
	return retval;
	CTL_DEV_ERROR:
		misc_deregister(&mm_dev);
	DEV_ERROR:
		pr_err("Could not connect to devices");
		vfree(user_view);
		return -ENODEV;
}

/**
 * mastermind_remove() - callback when this driver is removed
 * @pdev platform device driver data
 *
 * Return: Always 0
 */
static int mastermind_remove(struct platform_device *pdev)
{
	/* Copy the contents of your original mastermind_exit() here. */
	/* YOUR CODE HERE */
	pr_info("Freeing resources.\n");
	
	misc_deregister(&mm_dev);
	misc_deregister(&mm_ctl_dev);
	vfree(user_view);
	device_remove_file(&pdev->dev, &dev_attr_stats);
	return 0;
}

static struct platform_driver cs421_driver = {
	.driver = {
		   .name = "mastermind",
		   },
	.probe = mastermind_probe,
	.remove = mastermind_remove,
};

static struct platform_device *pdev;

/**
 * cs421_init() -  create the platform driver
 * This is needed so that the device gains a sysfs group.
 *
 * <strong>You do not need to modify this function.</strong>
 */
static int __init cs421_init(void)
{
	pdev = platform_device_register_simple("mastermind", -1, NULL, 0);
	if (IS_ERR(pdev))
		return PTR_ERR(pdev);
	return platform_driver_register(&cs421_driver);
}

/**
 * cs421_exit() - remove the platform driver
 * Unregister the driver from the platform bus.
 *
 * <strong>You do not need to modify this function.</strong>
 */
static void __exit cs421_exit(void)
{
	platform_driver_unregister(&cs421_driver);
	platform_device_unregister(pdev);
}

module_init(cs421_init);
module_exit(cs421_exit);

MODULE_DESCRIPTION("CS421 Mastermind Game++");
MODULE_LICENSE("GPL");
