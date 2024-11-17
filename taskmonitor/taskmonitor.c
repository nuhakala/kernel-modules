#include "asm-generic/errno-base.h"
#include "asm-generic/ioctl.h"
#include "linux/container_of.h"
#include "linux/debugfs.h"
#include "linux/fs.h"
#include "linux/kthread.h"
#include "linux/list.h"
#include "linux/mempool.h"
#include "linux/module.h"
#include "linux/printk.h"
#include "linux/sched.h"
#include "linux/seq_file.h"

#include "./commands.h"

MODULE_DESCRIPTION("Monitor given process pid");
MODULE_AUTHOR("nuhakala");
MODULE_LICENSE("GPL");

static int target = 1;
module_param(target, int, 0660);
MODULE_PARM_DESC(target, "Pid to monitor (default 1)");

// Struct definitions
struct task_monitor {
	struct pid *task;
	struct list_head samples;
	int list_length;
	struct mutex mutex;
	struct list_head list;
	pid_t target;
};

struct task_sample {
	u64 utime;
	u64 stime;
	unsigned long total;
	unsigned long stack;
	unsigned long data;
	struct list_head list;
	struct kref ref;
};

// FUNCTIONS
int monitor_pid(pid_t pid);
int monitor_fn(void *args);
// Handle samples
bool get_sample(struct task_monitor *tm, struct task_sample *sample);
struct task_sample *save_sample(struct task_monitor *task);
void clean_samples(void);
void clean_process(struct task_monitor *task);
// start stop thread
void start(void);
void stop(void);

// kref
static void sample_release(struct kref *kref);

// seq
static void *task_seq_start(struct seq_file *s, loff_t *pos);
static int task_debug_open(struct inode *inode, struct file *file);
static void *task_seq_start(struct seq_file *s, loff_t *pos);
static void *task_seq_next(struct seq_file *s, void *v, loff_t *pos);
static void task_seq_stop(struct seq_file *s, void *v);
static int task_seq_show(struct seq_file *s, void *v);

static ssize_t debug_write(struct file *file, const char __user *user_buf,
			   size_t size, loff_t *ppos);

// VARIABLES
static struct task_struct *kcpustat_thread;
struct kmem_cache *cache;

static struct dentry *debugfs_dentry;
static char *debug_file_name = "taskmonitor";
struct task_monitor *current_task;
struct task_sample *current_item;
static struct file_operations seq_fops = {
	.open = task_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
	.write = debug_write,
};
static const struct seq_operations seq_ops = { .start = task_seq_start,
					       .next = task_seq_next,
					       .stop = task_seq_stop,
					       .show = task_seq_show };

struct list_head tasks;

/* Init function */
static int __init taskmonitor_init(void)
{
	pr_info("nuhakala saapuu!!!\n");

	INIT_LIST_HEAD(&tasks);

	if (monitor_pid(target) == -1) {
		pr_info("Pidiä ei määritetty, keskeytetään");
		return 0;
	}

	// Initialize the slab allocator.
	cache = KMEM_CACHE(task_sample, 0);

	// Create slab memory pool
	mempool_create_slab_pool(200, cache);

	// Create debugfs file
	debugfs_dentry = debugfs_create_file(debug_file_name, 0660, NULL,
					     "saucrähuantaotnhsus",
					     // get_seq_fops(&task, target)
					     &seq_fops);

	// Start taking samples
	kcpustat_thread = kthread_run(monitor_fn, NULL, "monitor_fn");

	return 0;
}
module_init(taskmonitor_init);

/* Exit function */
static void __exit exit_module(void)
{
	if (kcpustat_thread)
		kthread_stop(kcpustat_thread);

	debugfs_remove(debugfs_dentry);

	clean_samples();

	pr_info("nuhakala poistuu!\n");
}
module_exit(exit_module);

struct task_sample *save_sample(struct task_monitor *task)
{
	struct task_sample *sample = kmem_cache_alloc(cache, 0);
	// Initialize reference counter
	kref_init(&sample->ref);

	if (IS_ERR(sample)) {
		return NULL;
	}
	INIT_LIST_HEAD(&sample->list);

	if (!get_sample(task, sample)) {
		pr_info("Prosessi on jo raa-asti murhattu :(\n");
	}

	// Save using mutex
	mutex_lock(&task->mutex);
	list_add(&sample->list, &task->samples);
	task->list_length++;
	mutex_unlock(&task->mutex);

	kref_get(&sample->ref);
	return sample;
}

void start(void)
{
	if (!kcpustat_thread)
		kcpustat_thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	pr_info("Aloitettu\n");
}

void stop(void)
{
	if (kcpustat_thread) {
		kthread_stop(kcpustat_thread);
		kcpustat_thread = NULL;
	}
	pr_info("Pysäytetty\n");
}

bool get_sample(struct task_monitor *tm, struct task_sample *sample)
{
	struct task_struct *prc = get_pid_task(tm->task, PIDTYPE_PID);
	bool res = false;

	if (pid_alive(prc)) {
		sample->utime = prc->utime;
		sample->stime = prc->stime;
		sample->total = prc->mm->total_vm;
		sample->stack = prc->mm->stack_vm;
		sample->data = prc->mm->data_vm;
		res = true;
	}

	put_task_struct(prc);
	return res;
}

int monitor_pid(pid_t pid)
{
	struct task_monitor *task =
		kmalloc(sizeof(struct task_monitor), GFP_KERNEL);
	task->target = pid;
	pr_info("Etsitään prosessio jolla on pid: %d\n", pid);
	task->task = find_get_pid(pid);
	if (task->task == NULL) {
		pr_info("Ei prosessia annetulla pidillä.\n");
		return -1;
	}
	pr_info("Prosessi löytyi!\n");
	mutex_init(&task->mutex);
	INIT_LIST_HEAD(&task->samples);
	list_add(&task->list, &tasks);
	return 0;
}

int monitor_fn(void *args)
{
	while (!kthread_should_stop()) {
		struct list_head *k;
		list_for_each(k, &tasks) {
			struct task_monitor *task =
				container_of(k, struct task_monitor, list);
			struct task_sample *sample = save_sample(task);
			if (sample == NULL) {
				pr_info("Näytteen tallennuksessa tapahtui häire\n");
				pr_info("Skipataan tämä kierros\n");
			} else {
				// pr_info("Näyte: %lu\n", sample->data);
				kref_put(&sample->ref, sample_release);
			}

			set_current_state(TASK_INTERRUPTIBLE);
		}
		schedule_timeout(0.5 * HZ);
	}
	return 0;
}

void clean_samples(void)
{
	// Remove the samples
	struct task_monitor *h, *l;
	list_for_each_entry_safe(h, l, &tasks, list) {
		clean_process(h);
		list_del(&h->list);
		kfree(h);
	}
}

void clean_process(struct task_monitor *task)
{
	struct task_sample *k, *n;
	mutex_lock(&task->mutex);
	list_for_each_entry_safe(k, n, &task->samples, list) {
		list_del(&k->list);
		// kfree(k);
		kref_put(&k->ref, sample_release);
	}
	mutex_unlock(&task->mutex);
	mutex_destroy(&task->mutex);
	put_pid(task->task);
}

static void sample_release(struct kref *kref)
{
	struct task_sample *sample =
		container_of(kref, struct task_sample, ref);
	kmem_cache_free(cache, sample);
}

static int task_debug_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &seq_ops);
}

static void *task_seq_start(struct seq_file *s, loff_t *pos)
{
	if (*pos != 0)
		return NULL;
	loff_t *spos = kmalloc(sizeof(loff_t), GFP_KERNEL);
	if (!spos)
		return NULL;
	*spos = *pos;
	current_task = list_first_entry(&tasks, struct task_monitor, list);
	current_item = list_first_entry(&current_task->samples,
					struct task_sample, list);
	return spos;
}

static void *task_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	loff_t *spos = v;
	// current_item = seq_list_next(v, &current_task->samples, v);
	bool last_sample =
		list_is_last(&current_item->list, &current_task->samples);
	bool last_task = list_is_last(&current_task->list, &tasks);
	if (last_task) {
		if (last_sample) {
			*pos = ++*spos;
			return NULL;
		} else {
			current_item = list_next_entry(current_item, list);
		}
	} else {
		if (last_sample) {
			current_task = list_next_entry(current_task, list);
			current_item = list_first_entry(&current_task->samples,
							struct task_sample,
							list);
		} else {
			current_item = list_next_entry(current_item, list);
		}
	}
	*pos = ++*spos;
	return spos;
}

static void task_seq_stop(struct seq_file *s, void *v)
{
	kfree(v);
}

static int task_seq_show(struct seq_file *s, void *v)
{
	loff_t *spos = v;
	// struct task_sample *item = container_of(current_item, struct task_sample, list);
	seq_printf(s, "position: %lld pid %d usr %llu sys %llu\n",
		   (long long)*spos, current_task->target, current_item->utime,
		   current_item->stime);
	return 0;
}

static ssize_t debug_write(struct file *file, const char __user *user_buf,
			   size_t size, loff_t *ppos)
{
	char buf[64];
	int buf_size;
	int ret;

	buf_size = min(size, (sizeof(buf) - 1));
	if (strncpy_from_user(buf, user_buf, buf_size) < 0)
		return -EFAULT;
	buf[buf_size] = 0;
	sscanf(buf, "%d", &ret);

	if (ret > 0) {
		monitor_pid(ret);
		return buf_size;
	} else {
		struct list_head *k;
		list_for_each(k, &tasks) {
			struct task_monitor *task =
				container_of(k, struct task_monitor, list);
			if (task->target == -ret) {
				clean_process(task);
				list_del(&task->list);
				kfree(task);
				return buf_size;
			}
		}
		return -EINVAL;
	}
}
