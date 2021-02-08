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
COMMON := -pthread $(DEBUG) -lm
LIBFLG := -c
TARGET := a.$(EXT)
MFILES := hospitales.c
EXAMPL := ejemplos

.PHONY: all

all:
	$(CC) $(COMMON) $(MFILES) -o $(TARGET)
run:
	@echo "Ejecutando Programa..."
	@command ./$(TARGET)

# Genera casos de pruebas para los tipos de datos:
tests: queue-tests refmap-tests

# Genera los casos de prueba para el tipo "Cola de Referencias":
queue-tests: generic-queue
	@echo [Q] Generando ejemplos de uso para la Cola de Referencias...
	$(CC) $(COMMON) $(EXAMPL)/simple_queue.c RefQueue.o -o $(EXAMPL)/simple_queue.$(EXT)
	$(CC) $(COMMON) $(EXAMPL)/shared_queue.c RefQueue.o -o $(EXAMPL)/shared_queue.$(EXT)
	$(CC) $(COMMON) $(EXAMPL)/quick_shared_queue.c RefQueue.o -o $(EXAMPL)/quick_shared_queue.$(EXT)
	$(CC) $(COMMON) $(EXAMPL)/tryget_queue.c RefQueue.o -o $(EXAMPL)/tryget_queue.$(EXT)
	@echo

# Genera los casos de prueba para el tipo "Mapa de Referencias":
refmap-tests: refmap
	@echo [M] Generando ejemplos de uso para el Mapa de Referencias...
	$(CC) $(COMMON) $(EXAMPL)/refmap-allocate.c RefMap.o -o $(EXAMPL)/refmap-allocate.$(EXT)
	$(CC) $(COMMON) $(EXAMPL)/refmap-debug.c RefMap.o -o $(EXAMPL)/refmap-debug.$(EXT)
	@echo

# Genera los Archivos Objetos de los Tipos de datos:
generic-queue:
	$(CC) $(COMMON) $(LIBFLG) RefQueue.c RefQueue.h
refmap:
	$(CC) $(COMMON) $(LIBFLG) RefMap.c RefMap.h
