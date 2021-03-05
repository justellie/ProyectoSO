# ------------------------------------------------------------------
# Elige la extensión del ejecutable según el Sistema Operativo:
# (para el caso en el que se utilice la librería pthread en windows)
#
# Colores: Azul(BL), Rojo(RL), Verde(GL)
# 		   Reiniciar Color(RE). Sólo en Unix/Linux.
BL  := \u001b[38;5;32m
RL  := \u001b[38;5;9m
GL  := \u001b[38;5;10m
RE  := \u001b[0m
# ------------------------------------------------------------------

# COMMON -> -lm:  enlaza librería matemática,
#           -lrt: enlaza con la librería de temporizadores a tiempo real.

# Variables de compilación
CC	   := gcc
DEBUG  := -g -O0
COMMON := -pthread $(DEBUG) -lm -lrt
MERGE  :=
LIBFLG := -c
TARGET := proyecto.out
TYPES  := Tipos
OBJS   := RefQueue.o RefMap.o definiciones.o main.o
EXAMPL := ejemplos
TXAMPL := $(TYPES)/$(EXAMPL)
ACTORS := $(wildcard actor_*.c)
OCTORS := $(ACTORS:%.c=%.o)

.PHONY: all

# TODO: Agregar los objetos generados por definiciones y actores
all: refmap generic-queue definiciones actores
	@echo -e "$(BL) [@] Generando archivo principal $(GL)($(TARGET))$(BL)...$(RE)"
	@echo -e "$(RL)$(OCTORS)$(RE)"
	$(CC) $(COMMON) $(LIBFLG) -c main.c definiciones.h actores.h
	$(CC) $(COMMON) $(OBJS) $(MERGE) $(OCTORS) -o $(TARGET)
run:
	@echo -e "$(BL) [|>] Ejecutando Programa...$(RE)"
	@command ./$(TARGET)
	@echo -e "$(GL)Hecho\n$(RE)"


# ------------------------------------------------------
# Crea los archivos de definiciones:
definiciones: refmap generic-queue
	@echo -e "$(BL) [D] Generando $(GL)definiciones$(BL): $(RE)"
	$(CC) $(COMMON) $(LIBFLG) definiciones.c definiciones.h
	@echo -e "$(GL)Hecho\n$(RE)"

# ------------------------------------------------------
# Crea los actores de definiciones:
actores: definiciones refmap generic-queue
	@echo -e "$(BL) [D] Generando $(GL)actores$(BL): $(RE)"
	$(CC) $(COMMON) $(LIBFLG) $(ACTORS)
	@echo -e "$(GL)Hecho\n$(RE)"

# ------------------------------------------------------
# Genera casos de pruebas para los tipos de datos:
tests: queue-tests refmap-tests

# Genera los casos de prueba para el tipo "Cola de Referencias":
queue-tests: generic-queue
	@echo -e "$(BL) [Q] Generando ejemplos de uso para la $(GL)Cola de Referencias$(RE)"
	$(CC) $(COMMON) $(TXAMPL)/simple_queue-long.c  RefQueue.o -o $(TXAMPL)/simple_queue-long.out
	$(CC) $(COMMON) $(TXAMPL)/simple_queue.c       RefQueue.o -o $(TXAMPL)/simple_queue.out
	$(CC) $(COMMON) $(TXAMPL)/shared_queue.c       RefQueue.o -o $(TXAMPL)/shared_queue.out
	$(CC) $(COMMON) $(TXAMPL)/quick_shared_queue.c RefQueue.o -o $(TXAMPL)/quick_shared_queue.out
	$(CC) $(COMMON) $(TXAMPL)/tryget_queue.c       RefQueue.o -o $(TXAMPL)/tryget_queue.out
	@echo -e "$(GL)Hecho\n$(RE)"

# Genera los casos de prueba para el tipo "Mapa de Referencias":
refmap-tests: refmap
	@echo -e "$(BL) [M] Generando ejemplos de uso para el $(GL)Mapa de Referencias$(RE)"
	$(CC) $(COMMON) $(TXAMPL)/refmap-allocate.c  RefMap.o -o $(TXAMPL)/refmap-allocate.out
	$(CC) $(COMMON) $(TXAMPL)/refmap-debug.c     RefMap.o -o $(TXAMPL)/refmap-debug.out
	$(CC) $(COMMON) $(TXAMPL)/refmap-debug-max.c RefMap.o -o $(TXAMPL)/refmap-debug-max.out
	@echo -e "$(GL)Hecho\n$(RE)"
# ------------------------------------------------------

misc-tests:
	@echo -e "$(BL) [?] Pruebas $(GL)Misceláneas$(BL)$(RE)"
	$(CC) $(COMMON) $(TXAMPL)/signals.c  -o $(TXAMPL)/signals.out
	$(CC) $(COMMON) $(TXAMPL)/timers.c   -o $(TXAMPL)/timers.out
	$(CC) $(COMMON) $(TXAMPL)/fin-cond.c -o $(TXAMPL)/fin-cond.out
	@echo -e "$(GL)Hecho\n$(RE)"




# ------------------------------------------------------
# Genera los Archivos Objetos de los Tipos de datos:
# 	>>> Los .o se generan en el archivo raíz.
# 		Ejemplo: compilar ./$(TYPES)/RefQueue.c --> genera --> ./RefQueue.o
generic-queue:
	@echo -e "$(BL) [Q] Creando $(GL)RefQueue$(BL):$(RE)"
	$(CC) $(COMMON) $(LIBFLG) $(TYPES)/RefQueue.c $(TYPES)/RefQueue.h
refmap:
	@echo -e "$(BL) [M] Creando $(GL)RefMap$(BL):$(RE)"
	$(CC) $(COMMON) $(LIBFLG) $(TYPES)/RefMap.c $(TYPES)/RefMap.h
# ------------------------------------------------------




# ------------------------------------------------------
# Métodos para limpiar los rastros de las compilaciones:
clean-all: clean-types clean-root
	@echo -e "$(GL)Hecho\n$(RE)"

# Limpieza:
clean-types:
	@echo -e "$(RL)[!] Eliminando archivos ejecutables de $(TXAMPL)$(RE)"
	rm -f $(TXAMPL)/*.out
	@echo -e "$(RL)[!] Eliminando archivos objetos de $(TYPES)$(RE)"
	rm -f $(TYPES)/*.h.gch
	@echo

clean-root:
	@echo -e "$(RL)[!] Eliminando archivos objetos de ./$(RE)"
	rm -f *.o *.h.gch
	@echo
# --------------------------------------------------------------------

