
MODULE_NAME ?= vrfm
MAP_SIZE ?= 134217728
CC=/usr/bin/cc

BUILD_DIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
MOD_OUTPUT_DIR =$(PWD)/bin
BUILD_DIR_MAKEFILE ?= $(PWD)/bin/Makefile
FLAGS = -O3 
#FLAGS = -O0 -DDEBUG -g

all: module sync 

debug: FLAGS = -O0 -DDEBUG -g
debug: all bin/test bin/test1


reinstall: uninstall install

sync:
	sync


bin/test: test.c
	${CC} $(FLAGS) -g test.c -o "bin/test"
bin/test1: test1.c
	${CC} $(FLAGS) -g test1.c -o "bin/test1" -I rfm2g/include -L rfm2g/api/ -l rfm2g

obj-m += $(MODULE_NAME).o 
 $(MODULE_NAME)-y += chdev.o main.o  vrfm_mmap.o net.o protocol.o crypt.o 

module: $(BUILD_DIR_MAKEFILE) 
	KCPPFLAGS=" -DMODULE_NAME=$(MODULE_NAME) -DMAP_SIZE=$(MAP_SIZE) $(FLAGS)"  	make -C $(BUILD_DIR) M=$(MOD_OUTPUT_DIR) src=$(PWD) CC=${CC} modules

$(BUILD_DIR):
	$(warning kernel header source not found, install with )
	$(warning sudo apt install linux-headers-$(shell uname -r) #ubuntu)
	$(warning sudo zypper install kernel-devel #suse)
	$(error stop)
	

$(MOD_OUTPUT_DIR):
	mkdir "$@"

$(BUILD_DIR_MAKEFILE): $(BUILD_DIR) $(MOD_OUTPUT_DIR)
	install -D  Makefile "$@"





default: module

clean:
	KCPPFLAGS=" -DMODULE_NAME=$(MODULE_NAME)" make -C $(BUILD_DIR) M=$(MOD_OUTPUT_DIR) src=$(PWD) clean
	rm $(BUILD_DIR_MAKEFILE)
	rm bin/test


install: all
	KCPPFLAGS=" -DMODULE_NAME=$(MODULE_NAME) -DMAP_SIZE=$(MAP_SIZE)" make -C $(BUILD_DIR)   M=$(MOD_OUTPUT_DIR) src=$(PWD) CC=${CC} modules
	@sudo insmod $(MOD_OUTPUT_DIR)/$(MODULE_NAME).ko netdevice=lo debug=0 rfmdevice=rfm2g0
	

uninstall:
	@lsmod | grep $(MODULE_NAME) && sudo rmmod $(MODULE_NAME) || echo "module is not loaded"
