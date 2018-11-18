# Makefile -- for scene settting
# 
# Written by Huadong Liu, http://www.cs.utk.edu/~hliu/
# 
# @(#) $Id: Makefile,v 1.4 2005/09/21 15:42:48 hliu Exp $
#

CC = gcc

CFLAGS = -g -Wall -DVERBOSE -I/usr/local/include/libxml2
LIBS = -lxml2 -lm

EXECUTABLES = scene_xml2ascii ascii2val_test

all: $(EXECUTABLES)

scene_xml2ascii: scene_xml2ascii.o base64.o
	$(CC) $(CFLAGS) -o scene_xml2ascii scene_xml2ascii.o base64.o ${LIBS}

ascii2val_test: ascii2val_test.o scene_ascii2val.o base64.o
	$(CC) $(CFLAGS) -o ascii2val_test ascii2val_test.o scene_ascii2val.o base64.o

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm -f core $(EXECUTABLES) *.o *.a
