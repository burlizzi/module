
MODULE_NAME ?= vrfm
DEVICE_NAME ?= rfm2g0
BUILD_DIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
MOD_OUTPUT_DIR =$(PWD)/bin
BUILD_DIR_MAKEFILE ?= $(PWD)/bin/Makefile

all: module


obj-m += $(MODULE_NAME).o 
 $(MODULE_NAME)-y += chdev.o main.o  mmap.o net.o protocol.o




module: $(BUILD_DIR_MAKEFILE)
	KCPPFLAGS="-DDEVICE_NAME=$(DEVICE_NAME) -DMODULE_NAME=$(MODULE_NAME)"  	make -C $(BUILD_DIR) M=$(MOD_OUTPUT_DIR) src=$(PWD)   modules


$(BUILD_DIR):
	$(warning kernel header source not found, install with )
	$(warning sudo apt install linux-headers-$(shell uname -r) #ubuntu)
	$(warning sudo zypper install kernel-devel #suse)
	$(error stop)
	

$(MOD_OUTPUT_DIR):
	mkdir "$@"
$(BUILD_DIR_MAKEFILE): $(BUILD_DIR) $(MOD_OUTPUT_DIR)
	install -D  Makefile "$@"

$(MOD_OUTPUT_DIR)/protocol.o:
	g++ -c protocol.cpp -o $@




default: module

clean:
	KCPPFLAGS="-DDEVICE_NAME=$(DEVICE_NAME) -DMODULE_NAME=$(MODULE_NAME)" make -C $(BUILD_DIR) M=$(MOD_OUTPUT_DIR) src=$(PWD) clean
	rm $(BUILD_DIR_MAKEFILE)


install: 
	KCPPFLAGS="-DDEVICE_NAME=$(DEVICE_NAME) -DMODULE_NAME=$(MODULE_NAME)" make -C $(BUILD_DIR)  M=$(MOD_OUTPUT_DIR) src=$(PWD)  modules_install
	#insmod $(MODULE_NAME).ko

uninstall:
	rmmod $(MODULE_NAME)
