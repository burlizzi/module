
MODULE_NAME ?= vrfm
DEVICE_NAME ?= rfm2g0
BUILD_DIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
MOD_OUTPUT_DIR =$(PWD)/bin
BUILD_DIR_MAKEFILE ?= $(PWD)/bin/Makefile


obj-m += $(MODULE_NAME).o 
 $(MODULE_NAME)-y += main.o  mmap.o protocol.o net.o


module: $(BUILD_DIR_MAKEFILE)
	KCPPFLAGS="-DDEVICE_NAME=$(DEVICE_NAME) -DMODULE_NAME=$(MODULE_NAME)"  	make -C $(BUILD_DIR) M=$(MOD_OUTPUT_DIR) src=$(PWD)   modules


$(BUILD_DIR_MAKEFILE): $(BUILD_DIR)
	touch "$@"

all: module

clean:
	KCPPFLAGS="-DDEVICE_NAME=$(DEVICE_NAME) -DMODULE_NAME=$(MODULE_NAME)" make -C $(BUILD_DIR) M=$(MOD_OUTPUT_DIR) src=$(PWD) clean

install: 
	KCPPFLAGS="-DDEVICE_NAME=$(DEVICE_NAME) -DMODULE_NAME=$(MODULE_NAME)" make -C $(BUILD_DIR)  M=$(MOD_OUTPUT_DIR) src=$(PWD)  modules_install
	#insmod $(MODULE_NAME).ko

uninstall:
	rmmod $(MODULE_NAME)
