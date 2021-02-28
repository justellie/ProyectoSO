# ------------------------------------------------------------------
# Elige la extensión del ejecutable según el Sistema Operativo:
# (para el caso en el que se utilice la librería pthread en windows)
EXT	   :=
ifeq ($(OS),Windows_NT)
	EXT +=exe
else
	EXT +=out
endif
# ------------------------------------------------------------------

# Variables de compilación
CC	   := gcc
DEBUG  := -g -O0
COMMON := -pthread $(DEBUG) -lm
LIBFLG := -c
TARGET := proyecto.$(EXT)
MFILES := hospitales.c
TYPES  := Tipos
EXAMPL := ejemplos
TXAMPL := $(TYPES)/$(EXAMPL)

.PHONY: all

all:
	$(CC) $(COMMON) $(MFILES) -o $(TARGET)
run:
	@echo "Ejecutando Programa..."
	@command ./$(TARGET)




# ------------------------------------------------------
# Genera casos de pruebas para los tipos de datos:
tests: queue-tests refmap-tests

# Genera los casos de prueba para el tipo "Cola de Referencias":
queue-tests: generic-queue
	@echo [Q] Generando ejemplos de uso para la Cola de Referencias...
	$(CC) $(COMMON) $(TXAMPL)/simple_queue.c       RefQueue.o -o $(TXAMPL)/simple_queue.$(EXT)
	$(CC) $(COMMON) $(TXAMPL)/shared_queue.c       RefQueue.o -o $(TXAMPL)/shared_queue.$(EXT)
	$(CC) $(COMMON) $(TXAMPL)/quick_shared_queue.c RefQueue.o -o $(TXAMPL)/quick_shared_queue.$(EXT)
	$(CC) $(COMMON) $(TXAMPL)/tryget_queue.c       RefQueue.o -o $(TXAMPL)/tryget_queue.$(EXT)
	@echo

# Genera los casos de prueba para el tipo "Mapa de Referencias":
refmap-tests: refmap
	@echo [M] Generando ejemplos de uso para el Mapa de Referencias...
	$(CC) $(COMMON) $(TXAMPL)/refmap-allocate.c  RefMap.o -o $(TXAMPL)/refmap-allocate.$(EXT)
	$(CC) $(COMMON) $(TXAMPL)/refmap-debug.c     RefMap.o -o $(TXAMPL)/refmap-debug.$(EXT)
	$(CC) $(COMMON) $(TXAMPL)/refmap-debug-max.c RefMap.o -o $(TXAMPL)/refmap-debug-max.$(EXT)
	@echo
# ------------------------------------------------------




# ------------------------------------------------------
# Genera los Archivos Objetos de los Tipos de datos:
# 	>>> Los .o se generan en el archivo raíz.
# 		Ejemplo: compilar ./$(TYPES)/RefQueue.c --> genera --> ./RefQueue.o
generic-queue:
	$(CC) $(COMMON) $(LIBFLG) $(TYPES)/RefQueue.c $(TYPES)/RefQueue.h
refmap:
	$(CC) $(COMMON) $(LIBFLG) $(TYPES)/RefMap.c $(TYPES)/RefMap.h
# ------------------------------------------------------




# ------------------------------------------------------
# Métodos para limpiar los rastros de las compilaciones:
clean-all: clean-types clean-root
	@echo Listo
	@echo

# Limpieza:
clean-types:
	@echo Eliminando archivos ejecutables de $(TXAMPL)
	rm -f $(TXAMPL)/*.$(EXT)
	@echo Eliminando archivos objetos de $(TYPES)
	rm -f $(TYPES)/*.h.gch
	@echo

clean-root:
	@echo Eliminando archivos objetos de ./
	rm -f *.o *.h.gch
	@echo
# --------------------------------------------------------------------

