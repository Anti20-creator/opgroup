.phony: all debug clean

SOURCES = src/opgroup.c
DESTDIR = .

all:
	@gcc -w  ${SOURCES} -o ${DESTDIR}/opgroup

debug:
	@gcc -w  -ggdb3 ${SOURCES} -o ${DESTDIR}/opgroup

clean:
	@rm ${DESTDIR}/opgroup
