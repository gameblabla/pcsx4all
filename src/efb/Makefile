obj-m += efb.o
all:
	make -C /home/steward/Github/gh_rs97_kernel-2.6.31.3 M=$(PWD) modules
	/home/steward/Github/buildroot/output/host/usr/bin/mipsel-linux-gcc app.c -o app -g
clean:
	make -C /home/steward/Github/gh_rs97_kernel-2.6.31.3 M=$(PWD) clean
	rm -f  app.o app
