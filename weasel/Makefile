KERNELDIR_LKP ?= path/to/your/kernel

obj-m += weasel.o

all :
	make -C $(KERNELDIR_LKP) M=$$PWD modules

clean:
	make -C $(KERNELDIR_LKP) M=$$PWD clean
