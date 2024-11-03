
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

//SYSCALL_DEFINE2(systemCallName,typeOfInput1,input1,typeOfInput2,input2)
//__user specify the input pointer point to user space memory
SYSCALL_DEFINE2(revstr, char __user *, input, size_t, len)
{
    char *k_buffer;
    int i;


    // Allocate kernel memory with len+1 bytes
    k_buffer = kmalloc(len + 1, GFP_KERNEL);

    // copy_from_user(to,from,bytes)
    copy_from_user(k_buffer, input, len);
    k_buffer[len] = '\0';

    printk(KERN_INFO "The original string: %s\n", k_buffer);

    // Reverse the string in kernel space
    for (i = 0; i < len / 2; i++) {
        char temp = k_buffer[i];
        k_buffer[i] = k_buffer[len - i - 1];
        k_buffer[len - i - 1] = temp;
    }

    printk(KERN_INFO "The reversed string: %s\n", k_buffer);

    //copy_to_user(to,from,bytes)
    copy_to_user(input, k_buffer, len);

    //free the kernel space memory
    kfree(k_buffer);
    return 0;
}

	
	
