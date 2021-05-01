.phony: all debug clean

SOURCES = src/opgroup.c
DESTDIR = .

all:
	@gcc -std=c99 -w  ${SOURCES} -o ${DESTDIR}/opgroup

debug:
	@gcc -std=c99 -w  -ggdb3 ${SOURCES} -o ${DESTDIR}/opgroup

clean:
	@rm ${DESTDIR}/opgroup
