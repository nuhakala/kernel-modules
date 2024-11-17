/**
 * This module prints information aboud dentry cache when it is started.
 * Creates folder and three files to /proc/weasel/ which print the lines dcache.
 */

#include "asm-generic/errno-base.h"
#include "linux/dcache.h"
#include "linux/fs.h"
#include "linux/gfp_types.h"
#include "linux/list_bl.h"
#include "linux/module.h"
#include "linux/hashtable.h"
#include "linux/percpu-defs.h"
#include "linux/proc_fs.h"
#include "linux/seq_file.h"
#include "linux/uaccess.h"

MODULE_DESCRIPTION("Module for printing dentry info");
MODULE_AUTHOR("nuhakala");
MODULE_LICENSE("GPL");

int length;

void print_dcache(void)
{
	struct hlist_bl_node *pos;
	struct dentry *dentry;
	int max = 0; // longest bucket list
	int total = 0; // total number of dentries
	for (int i = 0; i < length; i++) {
		int list_size = 0;
		hlist_bl_for_each_entry(dentry, pos, dentry_hashtable + i,
					d_hash) {
			list_size++;
			total++;
		}
		if (list_size > max)
			max = list_size;
	}

	pr_info("hash shift: %d\n", d_hash_shift);
	pr_info("Pointteri: %p, bucketteja %d kpl, yhteensä entryjä %d, pisin %d\n",
		dentry_hashtable, length, total, max);
	pr_info("Dentryn koko: %lu tavua, memory footprint: %lu kb\n",
		sizeof(struct dentry), sizeof(struct dentry) * total / 1000);
}

ssize_t whoami_read(struct file *file, char __user *ubuf, size_t count,
		    loff_t *ppos)
{
	char *string = "I'm a weasel!\n";
	return simple_read_from_buffer(ubuf, count, ppos, string,
				       strlen(string));
}
struct proc_ops whoami_pocs = {
	.proc_read = whoami_read,
};

static int dcache_show(struct seq_file *m, void *p)
{
	// Number of buckets
	char *tmp = kmalloc(200 * sizeof(char), GFP_KERNEL);

	struct hlist_bl_node *pos;
	struct dentry *dentry;
	for (int i = 0; i < length; i++) {
		hlist_bl_for_each_entry(dentry, pos, dentry_hashtable + i,
					d_hash) {
			seq_printf(m, "%s\n",
				   dentry_path_raw(dentry, tmp, 200));
		}
	}
	return 0;
}
int dcache_open(struct inode *node, struct file *file)
{
	return single_open(file, dcache_show, NULL);
}
struct proc_ops dcache_pocs = { .proc_open = dcache_open,
				.proc_read = seq_read,
				.proc_lseek = seq_lseek,
				.proc_release = single_release };

static int pwd_show(struct seq_file *m, void *p)
{
	char *tmp = kmalloc(200 * sizeof(char), GFP_KERNEL);

	struct hlist_bl_node *pos;
	struct dentry *dentry;
	for (int i = 0; i < length; i++) {
		hlist_bl_for_each_entry(dentry, pos, dentry_hashtable + i,
					d_hash) {
			if (dentry->d_inode == NULL) {
				seq_printf(m, "%s\n",
					   dentry_path_raw(dentry, tmp, 200));
			}
		}
	}
	return 0;
}
int pwd_open(struct inode *node, struct file *file)
{
	return single_open(file, pwd_show, NULL);
}
struct proc_ops pwd_pocs = { .proc_open = pwd_open,
			     .proc_read = seq_read,
			     .proc_lseek = seq_lseek,
			     .proc_release = single_release };

char *folder_name = "weasel";
struct proc_dir_entry *folder;
struct proc_dir_entry *whoami;
struct proc_dir_entry *dcache;
struct proc_dir_entry *pwd;

/* Init function */
static int __init my_init(void)
{
	pr_info("nuhakala saapuu!!!\n");

	// Number of buckets
	length = 1 << (d_hash_shift + 2);

	// Task 2: 2-3
	print_dcache();

	folder = proc_mkdir(folder_name, NULL);
	whoami = proc_create("whoami", 0440, folder, &whoami_pocs);
	dcache = proc_create("dcache", 0440, folder, &dcache_pocs);
	pwd = proc_create("pwd", 0440, folder, &pwd_pocs);

	/* Question 6:
	 * the path seems to be /usr/bin only
	 * These enties are in the dcache, because the fs tries to look for
	 * files named foo in the path.*/

	pr_info("Weasel kansio luotu");

	return 0;
}
module_init(my_init);

/* Exit function */
static void __exit exit_module(void)
{
	remove_proc_subtree(folder_name, NULL);
	pr_info("nuhakala poistuu!\n");
}
module_exit(exit_module);
