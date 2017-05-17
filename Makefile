IDIR =./include
CC=cc
CFLAGS=-D_GNU_SOURCE -ansi -Wall -g -I$(IDIR)

SRC_DIR = ./src

ODIR=obj

_DEPS = escalonador.h gerente_de_execucao.h shutdown.h executa_postergado.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = escalonador.o gerente_de_execucao.o shutdown.o executa_postergado.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

escalonador: $(SRC_DIR)/escalonador.c $(IDIR)/escalonador.h
	$(CC) -o $@ $(SRC_DIR)/escalonador.c $(CFLAGS)

gerente_de_execucao: $(SRC_DIR)/gerente_de_execucao.c $(IDIR)/gerente_de_execucao.h
	$(CC) -o $@ src/gerente_de_execucao.c $(CFLAGS)

executa_postergado: $(SRC_DIR)/executa_postergado.c $(IDIR)/executa_postergado.h	
	$(CC) -o $@ $(SRC_DIR)/executa_postergado.c $(CFLAGS)

shutdown: $(SRC_DIR)/shutdown.c $(IDIR)/shutdown.h
	$(CC) -o $@ $(SRC_DIR)/shutdown.c $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
