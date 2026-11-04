#ifdef __cplusplus
extern "C" int printf( const char *, ...);
#define WASM_EXPORT __attribute__((visibility("default"))) extern "C"
#else
extern int printf( const char *, ...);
#define WASM_EXPORT __attribute__((visibility("default")))
#endif

#include <stddef.h>
#include <stdint.h>

#include "../external_modules/wasm_libs/include/wasm_libs_runtime.h"

static const double a = 17.27;
static const double b = 237.7;

double alpha(double temp, double hum) {
    return (a * temp) / (b + temp) + math_log(hum / 100);
}

// reads a SAUL device and returns its value (sum of all dimensions) in units of 10^power
// return value INT32_MIN means error while reading value
// return value INT32_MAX means value out of int32_t bounds
static int32_t reg_read(saul_reg_t dev, int8_t power) {
    saul_phydat_t phydat = { 0 };

    int dim = saul_reg_read(dev, &phydat);
    if (dim < 0) {
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

WASM_EXPORT int main(int argc, char **argv) {
    saul_reg_t temp_dev = saul_reg_find_type_and_name(SAUL_SENSE_TEMP, "bme680");
    saul_reg_t hum_dev = saul_reg_find_type_and_name(SAUL_SENSE_HUM, "bme680");
    if (!temp_dev || !hum_dev) return 1;

    int32_t temp = reg_read(temp_dev, 0);
    int32_t hum = reg_read(hum_dev, 0);
    double dew_point = (b * alpha(temp, hum)) / (a - alpha(temp, hum));

    printf("Temperature: ");
    io_printi32(temp);
    printf(" °C\nHumidity: ");
    io_printi32(hum);
    printf(" %\nDew point: ");
    io_printd(dew_point);
    printf(" °C\n");

    return 0;
}