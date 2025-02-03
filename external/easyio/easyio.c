#include <stdio.h>

#include "easyio.h"

void easyio_prints(const char* s) {
	printf("%s", s);
}

void easyio_printsl(const char* s, size_t l) {
	for (size_t i = 0; i < l; ++i)
		printf("%c", s[i]);
}

void easyio_printi(int i) {
	printf("%d", i);
}

void easyio_printc(char c) {
	printf("%c", c);
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