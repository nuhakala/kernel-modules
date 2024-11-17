KERNELDIR_LKP ?= path/to/your/kernel

obj-m += taskmonitor.o
# CFLAGS_prdebug.o := -DDEBUG

all :
	make -C $(KERNELDIR_LKP) M=$$PWD modules

user : user.c
	gcc -static -Wall -o user.o user.c

mod :
	mknod /dev/taskmonitor c ${major} 0

clean:
	make -C $(KERNELDIR_LKP) M=$$PWD clean
