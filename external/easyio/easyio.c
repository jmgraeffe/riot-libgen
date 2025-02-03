#include <stdio.h>

#include "easyio.h"

void easyio_prints(const char* s) {
	printf("%s", s);
}

void easyio_printi(int i) {
	printf("%d", i);
}

void easyio_puts(const char* s) {
	puts(s);
}

void easyio_puti(int i) {
	printf("%d\n", i);
}