obj-m += printer.o

all:    
	make -C /home/elinux1/Desktop/doc/bbb/source/linux-3.14.26-g2489c02 M=$(PWD) modules
clean:    
	make -C /home/elinux1/Desktop/doc/bbb/source/linux-3.14.26-g2489c02 M=$(PWD) clean

