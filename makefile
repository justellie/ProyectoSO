CC	   := gcc
DEBUG  := -g -O0
COMMON := -pthread $(DEBUG)
LIBFLG := -c
TARGET := a.out
MFILES := hospitales.c
.PHONY: all

all:
	$(CC) $(COMMON) $(MFILES) -o $(TARGET)
run:
	@echo "Ejecutando Programa..."
	@command ./$(TARGET)


queue-tests: generic-queue
	$(CC) $(COMMON) ejemplos/simple_queue.c Queue.o -o ejemplos/simple_queue.out

generic-queue:
	$(CC) $(COMMON) $(LIBFLG) Queue.c Queue.h
