IDIR =./include
CC=gcc
CFLAGS=-D_GNU_SOURCE -ansi -Wall -g -I$(IDIR)

SRC_DIR = ./src

ODIR=obj

BDIR=bin

escalonador: $(SRC_DIR)/escalonador.c $(IDIR)/escalonador.h $(IDIR)/utils.h 
	$(CC) -o $(BDIR)/$@ $(SRC_DIR)/escalonador.c $(CFLAGS)

gerente_de_execucao: $(SRC_DIR)/gerente_de_execucao.c $(IDIR)/gerente_de_execucao.h $(IDIR)/utils.h
	$(CC) -o $(BDIR)/$@ src/gerente_de_execucao.c $(CFLAGS)

executa_postergado: $(SRC_DIR)/executa_postergado.c $(IDIR)/executa_postergado.h $(IDIR)/utils.h
	$(CC) -o $(BDIR)/$@ $(SRC_DIR)/executa_postergado.c $(CFLAGS)

shutdown: $(SRC_DIR)/shutdown.c $(IDIR)/shutdown.h $(IDIR)/utils.h
	$(CC) -o $(BDIR)/$@ $(SRC_DIR)/shutdown.c $(CFLAGS)

dummy: dummy.c $(IDIR)/utils.h
	$(CC) -o $(BDIR)/$@ dummy.c $(CFLAGS)

all: escalonador gerente_de_execucao executa_postergado shutdown dummy

.PHONY: clean

clean:
	rm -f $(BDIR)/* $(ODIR)/*.o *~ core $(INCDIR)/*~ 
