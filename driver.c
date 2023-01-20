#include <linux/kernel.h> /* We're doing kernel work */
#include <linux/module.h> /* Specifically, a module */
#include <linux/uaccess.h> /* copy_to_user, copy_from_user */
#include <linux/proc_fs.h> /* Necessary because we use the proc fs */
#include <linux/pci.h> /* pci struct */
#include <linux/fs.h> /* mutex, inode*/

#define BUF_MAX_SIZE 4096
#define SEGMENTS_MAX 10
#define PROC_NAME "lab_driver"

MODULE_LICENSE("GPL");

static struct proc_dir_entry* parent;
static int struct_id = 0;
static int pid = 0;

static int __init lab_driver_init(void);
static void __exit lab_driver_exit(void);

static int lab_driver_open(struct inode *inode, struct file *file);
static int lab_driver_release(struct inode *inode, struct file *file);
static ssize_t lab_driver_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos);
static ssize_t lab_driver_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos);

struct mutex etx_mutex;

static struct file_operations fops = {
    .open = lab_driver_open,
    .read = lab_driver_read,
    .write = lab_driver_write,
    .release = lab_driver_release
};


static ssize_t read_pci_dev(char __user *buffer){
    char current_buffer[BUF_MAX_SIZE];
    ssize_t len;
    static struct pci_dev *dev;

    len = sprintf(current_buffer, "pci_dev struct:\n");
    while ((dev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, dev))){
        printk(KERN_INFO "************************** \n"
            "device = %hu \n"
            "pci_name = %s \n"
            "vendor = %hu \n",
            dev->device, pci_name(dev), dev->vendor);
    	len += sprintf(current_buffer + len, "************************** \n"
            "device = %hu \n"
            "pci_name = %s \n"
            "vendor = %hu \n",
            dev->device, pci_name(dev), dev->vendor);
    	if (dev->slot != NULL){
    		len += sprintf(current_buffer + len, "slot_name = %s\n", pci_slot_name(dev->slot));
    	}
    	len += sprintf(current_buffer + len, "************************** \n\n");
    }

    if (copy_to_user(buffer, current_buffer, len)){
        return -EFAULT;
    }

    return len;
}

static ssize_t read_inode(char __user *buffer){
    char current_buffer[BUF_MAX_SIZE];
    ssize_t len;
    struct task_struct* task;

    len = sprintf(current_buffer, "inode struct:\n");
    task = get_pid_task(find_vpid(pid), PIDTYPE_PID);
    if(task == NULL){
        printk(KERN_INFO "Task is not initialized \n");
        return -EFAULT;
    }
    else{
        if(!task->active_mm){
            printk(KERN_INFO "Task is not mapped \n");
            return -EFAULT;
        }
        else{
            struct vm_area_struct* vm_area;
	    struct inode *inode;
	    unsigned long vm_start;
	    unsigned long vm_end;
	    int segments_amount = 0;
	    vm_area = task -> active_mm -> mmap;
            while (vm_area && segments_amount < SEGMENTS_MAX) {
                if (vm_area -> vm_file) {
		    segments_amount += 1;
		    vm_start = vm_area -> vm_start;
		    vm_end = vm_area -> vm_end; 
                    inode = vm_area -> vm_file -> f_inode;
                    printk(KERN_INFO "************************** \n"
		       "vm_start = %x \n"
		       "vm_end = %x \n"
                       "i_ino = %lu \n"
                       "i_mode = %05o \n"
                       "i_size = %lld \n"
                       "i_blocks = %llu \n"
                       "i_uid = %d \n"
		       "i_gid = %d \n"
                       " ************************** \n",
                        vm_start, vm_end, inode->i_ino, inode->i_mode, inode->i_size, inode->i_blocks, inode->i_uid.val, inode->i_gid.val);
                    len += sprintf(current_buffer+len, "************************** \n"
                        "vm_start = %lu \n"
			"vm_end = %lu \n"
			"i_ino = %lu \n"
                        "i_mode = %05o \n"
                        "i_size = %lld \n"
                        "i_blocks = %llu \n"
                        "i_uid = %d \n"
			"i_gid = %d \n"
                        "************************** \n",
                        vm_start, vm_end, inode->i_ino, inode->i_mode, inode->i_size, inode->i_blocks, inode->i_uid.val, inode->i_gid.val);
                }
                vm_area = vm_area -> vm_next;
            }
        }
    }
    if (copy_to_user(buffer, current_buffer, len)){
        return -EFAULT;
    }
    return len;
}

static ssize_t lab_driver_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos) {
    ssize_t len;

    printk(KERN_INFO "Read function called \n");

    if (*ppos > 0 || count < BUF_MAX_SIZE) {
        return 0;
    }

    switch(struct_id){
    	case 0:
    		len = read_pci_dev(buffer);
    		break;
    	case 1:
    		len = read_inode(buffer);
    		break;
    }

    *ppos = len;
    printk(KERN_INFO "Read function over \n");
    return len;
}

static ssize_t lab_driver_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos) {
    char current_buffer[BUF_MAX_SIZE];
    ssize_t len;

    printk(KERN_INFO "Write function called \n");
    if (*ppos > 0 || count > BUF_MAX_SIZE){
        return -EFAULT;
    }

    if(copy_from_user(current_buffer, buffer, count) ) {
        return -EFAULT;
    }

    sscanf(current_buffer, "%d %d", &struct_id, &pid);
    len = strlen(current_buffer);
    *ppos = len;
    printk(KERN_INFO "Write function over \n");
    return len;
}

static int lab_driver_open(struct inode *inode, struct file *file) {
    mutex_lock(&etx_mutex);
    printk(KERN_INFO "Open function called \n");
    return 0;
}

static int lab_driver_release(struct inode *inode, struct file *file) {
    mutex_unlock(&etx_mutex);
    printk(KERN_INFO "Close function called \n");
    return 0;
}

static int __init lab_driver_init(void) {
    mutex_init(&etx_mutex);
    /* Create file in "/proc" */
    parent = proc_create(PROC_NAME, 0666, parent, &fops);

    if(parent == NULL) {
        proc_remove(parent);
        printk(KERN_INFO "Error creating proc entry \n");
        return -1;
    }

    printk(KERN_INFO "Driver installation succeed \n");
    return 0;
}


static void __exit lab_driver_exit(void) {
    /* Remove module from /proc */
    proc_remove(parent);
    printk(KERN_INFO "Device deleted \n");
}

module_init(lab_driver_init);
module_exit(lab_driver_exit);
