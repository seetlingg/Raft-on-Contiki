CONTIKI_PROJECT = raft_node
PROJECT_SOURCEFILES += raft.c
APPS+=powertrace
all: $(CONTIKI_PROJECT)
CONTIKI = ../..
CONTIKI_WITH_RIME = 1
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\" -g
include $(CONTIKI)/Makefile.include
