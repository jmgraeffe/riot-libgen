#include <stdio.h>

#include "easyio.h"

void easyio_prints(const char* s) {
	printf("%s", s);
}

void easyio_printsl(const char* s, size_t l) {
	printf("%.*s", l, s);
}

void easyio_printi(int i) {
	printf("%d", i);
}

void easyio_printc(char c) {
	printf("%c", c);
}

void easyio_printd(double d) {
	printf("%f", d);
}

void easyio_puts(const char* s) {
	puts(s);
}

void easyio_puti(int i) {
	printf("%d\n", i);
}

void easyio_putc(char c) {
	printf("%c\n", c);
}

void easyio_putd(double d) {
	printf("%f\n", d);
}