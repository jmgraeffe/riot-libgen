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

WASM_EXPORT int main(int argc, char **argv) {
    easysaul_reg_t temp_dev = easysaul_reg_find_type_and_name(EASYSAUL_SENSE_TEMP, "bme680");
    easysaul_reg_t hum_dev = easysaul_reg_find_type_and_name(EASYSAUL_SENSE_HUM, "bme680");
    if (!temp_dev || !hum_dev) return 1;

    double temp = easysaul_reg_read(temp_dev);
    double hum = easysaul_reg_read(hum_dev);
    double dew_point = (b * alpha(temp, hum)) / (a - alpha(temp, hum));

    printf("Temperature: ");
    io_printd(temp);
    printf(" °C\nHumidity: ");
    io_printd(hum);
    printf(" %\nDew point: ");
    io_printd(dew_point);
    printf(" °C\n");

    return 0;
}