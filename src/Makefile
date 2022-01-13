CONTIKI_PROJECT = raft_node
PROJECT_SOURCEFILES += raft.c
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"


TARGET = sky
CONTIKI_WITH_RIME = 1


COMPILERPATH = /usr/bin
CC = $(COMPILERPATH)/arm-none-eabi-gcc
OBJCOPY = $(COMPILERPATH)/arm-none-eabi-objcopy


CONTIKI = ../..
include $(CONTIKI)/Makefile.include

contikclean:
	make clean $(CONTIKI)

all: contikclean $(CONTIKI_PROJECT)
