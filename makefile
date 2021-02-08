# Extensión según el sistema operativo:
EXT	   :=
ifeq ($(OS),Windows_NT)
	EXT +=exe
else
	EXT +=out
endif

# Variables de compilación
CC	   := gcc
DEBUG  := -g -O0
COMMON := -pthread $(DEBUG)
LIBFLG := -c
TARGET := a.$(EXT)
MFILES := hospitales.c

.PHONY: all

all:
	$(CC) $(COMMON) $(MFILES) -o $(TARGET)
run:
	@echo "Ejecutando Programa..."
	@command ./$(TARGET)


queue-tests: generic-queue
	@echo Generando ejemplos de uso para la Cola de Referencias...
	$(CC) $(COMMON) ejemplos/simple_queue.c RefQueue.o -o ejemplos/simple_queue.$(EXT)
	$(CC) $(COMMON) ejemplos/shared_queue.c RefQueue.o -o ejemplos/shared_queue.$(EXT)
	$(CC) $(COMMON) ejemplos/quick_shared_queue.c RefQueue.o -o ejemplos/quick_shared_queue.$(EXT)
	$(CC) $(COMMON) ejemplos/tryget_queue.c RefQueue.o -o ejemplos/tryget_queue$(EXT)

generic-queue:
	$(CC) $(COMMON) $(LIBFLG) RefQueue.c RefQueue.h
