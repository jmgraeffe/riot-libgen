#include <stdio.h>
#include <math.h>
#include <errno.h>

#include "phydat.h"
#include "easysaul.h"

// return value INT32_MIN means error while reading value (check errno)
// return value INT32_MAX means value out of int32_t bounds
int32_t easysaul_reg_read(saul_reg_t* dev, int8_t power) {
	phydat_t phydat = { 0 };

	int dim = saul_reg_read(dev, &phydat);
	if (dim < 0) {
		errno = dim;
		return INT32_MIN;
	}

	int64_t res = 0;
	for (int i = 0; i < dim; ++i) {
		res += phydat.val[i];
	}

	power += phydat.scale;
	for (int i = 0; i < power; ++i) {
		res *= 10;	
	}
	for (int i = power; i < 0; ++i) {
		res /= 10;
	}

	if (res > INT32_MAX || res < INT32_MIN) return INT32_MAX;
	return (int32_t) res;
}

uint8_t easysaul_reg_unit_get(saul_reg_t* dev) {
	phydat_t phydat = { 0 };

	int dim = saul_reg_read(dev, &phydat);
	if (dim < 0) {
		errno = dim;
		return UNIT_UNDEF;
	}

	return phydat.unit;
}