CONTIKI_PROJECT = raft_node
PROJECT_SOURCEFILES += raft.c

TARGET = sky
CONTIKI_WITH_IPV6 = 1


COMPILERPATH = /usr/bin
CC = $(COMPILERPATH)/arm-none-eabi-gcc
OBJCOPY = $(COMPILERPATH)/arm-none-eabi-objcopy


CONTIKI = ../..
include $(CONTIKI)/Makefile.include

contikclean:
	make clean $(CONTIKI)

all: contikclean $(CONTIKI_PROJECT)
