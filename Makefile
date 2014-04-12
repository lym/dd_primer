# Makefile - makefile for ofd.c

# if KERNELRELEASE is defined, we've been invoked from the kernel build
# system and can use its language.
ifneq (${KERNELRELEASE},)
	obj-m := ofd.o mod_par.o sleepy.o jiffies_test.o jit.o jit_cur_time.o
# Otherwise we were called directly from the command line.
# Invoke the kernel build system.
else
	# KERNEL_SOURCE := /usr/include/linux
	KERNEL_SOURCE := /lib/modules/3.0.0-32-generic/build/
	PWD := $(shell pwd)
default:
	${MAKE} -C ${KERNEL_SOURCE} SUBDIRS=${PWD} modules
clean:
	${MAKE} -C ${KERNEL_SOURCE} SUBDIRS=${PWD} clean
endif
