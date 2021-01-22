CC	   := gcc
DEBUG  := -g -O3
COMMON := -pthread $(DEBUG)
TARGET := a.out
MFILES := hospitales.c
.PHONY: all

all:
	$(CC) $(COMMON) $(MFILES) -o $(TARGET)
run:
	@echo "Ejecutando Programa..."
	@command ./$(TARGET)
