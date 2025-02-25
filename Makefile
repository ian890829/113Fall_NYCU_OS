obj-m += kfetch_mod_313553052.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

load:
	sudo insmod kfetch_mod_313553052.ko

unload:
	sudo rmmod kfetch_mod_312553052