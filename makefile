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
	@echo Generando ejemplos de uso para la Cola de Referencias...
	$(CC) $(COMMON) ejemplos/simple_queue.c RefQueue.o -o ejemplos/simple_queue.out
	$(CC) $(COMMON) ejemplos/shared_queue.c RefQueue.o -o ejemplos/shared_queue.out
	$(CC) $(COMMON) ejemplos/quick_shared_queue.c RefQueue.o -o ejemplos/quick_shared_queue.out
	$(CC) $(COMMON) ejemplos/tryget_queue.c RefQueue.o -o ejemplos/tryget_queue.out

generic-queue:
	$(CC) $(COMMON) $(LIBFLG) RefQueue.c RefQueue.h
