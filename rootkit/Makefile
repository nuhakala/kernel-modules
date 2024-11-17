KERNELDIR_LKP ?= path/to/your/kernel

obj-m += rootkit.o
CFLAGS_rootkit.o := -DDEBUG

all :
	make -C $(KERNELDIR_LKP) M=$$PWD modules

clean:
	make -C $(KERNELDIR_LKP) M=$$PWD clean
