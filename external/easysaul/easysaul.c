#include <stdio.h>
#include <math.h>
#include <errno.h>

#include "phydat.h"
#include "easysaul.h"

double easysaul_reg_read(saul_reg_t* dev) {
	phydat_t phydat = { 0 };

	int dim = saul_reg_read(dev, &phydat);
	if (dim < 0) {
		errno = dim;
		return NAN;
	}

	double res = 0;
	for (int i = 0; i < dim; ++i) {
		res += phydat.val[i] * pow(10, phydat.scale);
	}
	return res;
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