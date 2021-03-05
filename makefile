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
CC     := gcc
DEBUG  := -g -O0 -Wall
COMMON := -pthread $(DEBUG) -lm -lrt --std=gnu99
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
	$(CC) $(LIBFLG) -c main.c definiciones.h actores.h $(COMMON) 
	$(CC) $(OBJS) $(OCTORS) -o $(TARGET)               $(COMMON) 
run:
	@echo -e "$(BL) [|>] Ejecutando Programa...$(RE)"
	@command ./$(TARGET)
	@echo -e "$(GL)Hecho\n$(RE)"

force: clean-all all

# ------------------------------------------------------
# Crea los archivos de definiciones:
definiciones: refmap generic-queue
	@echo -e "$(BL) [D] Generando $(GL)definiciones$(BL): $(RE)"
	$(CC) $(LIBFLG) definiciones.c definiciones.h $(COMMON) 
	@echo -e "$(GL)Hecho\n$(RE)"

# ------------------------------------------------------
# Crea los actores de definiciones:
actores: definiciones refmap generic-queue
	@echo -e "$(BL) [D] Generando $(GL)actores$(BL): $(RE)"
	$(CC) $(LIBFLG) $(ACTORS) $(COMMON) 
	@echo -e "$(GL)Hecho\n$(RE)"

# ------------------------------------------------------
# Genera casos de pruebas para los tipos de datos:
tests: queue-tests refmap-tests

# Genera los casos de prueba para el tipo "Cola de Referencias":
queue-tests: generic-queue
	@echo -e "$(BL) [Q] Generando ejemplos de uso para la $(GL)Cola de Referencias$(RE)"
	$(CC) $(TXAMPL)/simple_queue-long.c  RefQueue.o -o $(TXAMPL)/simple_queue-long.out  $(COMMON) 
	$(CC) $(TXAMPL)/simple_queue.c       RefQueue.o -o $(TXAMPL)/simple_queue.out       $(COMMON) 
	$(CC) $(TXAMPL)/shared_queue.c       RefQueue.o -o $(TXAMPL)/shared_queue.out       $(COMMON) 
	$(CC) $(TXAMPL)/quick_shared_queue.c RefQueue.o -o $(TXAMPL)/quick_shared_queue.out $(COMMON) 
	$(CC) $(TXAMPL)/tryget_queue.c       RefQueue.o -o $(TXAMPL)/tryget_queue.out       $(COMMON) 
	@echo -e "$(GL)Hecho\n$(RE)"

# Genera los casos de prueba para el tipo "Mapa de Referencias":
refmap-tests: refmap
	@echo -e "$(BL) [M] Generando ejemplos de uso para el $(GL)Mapa de Referencias$(RE)"
	$(CC) $(TXAMPL)/refmap-allocate.c  RefMap.o -o $(TXAMPL)/refmap-allocate.out  $(COMMON) 
	$(CC) $(TXAMPL)/refmap-debug.c     RefMap.o -o $(TXAMPL)/refmap-debug.out     $(COMMON) 
	$(CC) $(TXAMPL)/refmap-debug-max.c RefMap.o -o $(TXAMPL)/refmap-debug-max.out $(COMMON) 
	@echo -e "$(GL)Hecho\n$(RE)"
# ------------------------------------------------------

misc-tests:
	@echo -e "$(BL) [?] Pruebas $(GL)Misceláneas$(BL)$(RE)"
	$(CC) $(TXAMPL)/signals.c  -o $(TXAMPL)/signals.out  $(COMMON) 
	$(CC) $(TXAMPL)/timers.c   -o $(TXAMPL)/timers.out   $(COMMON) 
	$(CC) $(TXAMPL)/fin-cond.c -o $(TXAMPL)/fin-cond.out $(COMMON) 
	@echo -e "$(GL)Hecho\n$(RE)"




# ------------------------------------------------------
# Genera los Archivos Objetos de los Tipos de datos:
# 	>>> Los .o se generan en el archivo raíz.
# 		Ejemplo: compilar ./$(TYPES)/RefQueue.c --> genera --> ./RefQueue.o
generic-queue:
	@echo -e "$(BL) [Q] Creando $(GL)RefQueue$(BL):$(RE)"
	$(CC) $(LIBFLG) $(TYPES)/RefQueue.c $(TYPES)/RefQueue.h $(COMMON) 
refmap:
	@echo -e "$(BL) [M] Creando $(GL)RefMap$(BL):$(RE)"
	$(CC) $(LIBFLG) $(TYPES)/RefMap.c $(TYPES)/RefMap.h $(COMMON) 
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

