#ifdef __cplusplus
extern "C" int printf( const char *, ...);
#define WASM_EXPORT __attribute__((visibility("default"))) extern "C"
#else
extern int printf( const char *, ...);
#define WASM_EXPORT __attribute__((visibility("default")))
#endif

#include <stddef.h>
#include <stdint.h>

#define EASYSAUL_SENSE_TEMP 130
#define EASYSAUL_SENSE_HUM 131

static const double a = 17.27;
static const double b = 237.7;

void io_printd(double d);
void io_printi32(int32_t i);
double math_log(double arg);
int32_t easysaul_reg_find_and_read(uint8_t type, const char* name, int8_t power);

double alpha(double temp, double hum) {
    return (a * temp) / (b + temp) + math_log(hum / 100);
}

WASM_EXPORT int main(int argc, char **argv) {
    int32_t temp = easysaul_reg_find_and_read(EASYSAUL_SENSE_TEMP, "bme680", 0);
    int32_t hum = easysaul_reg_find_and_read(EASYSAUL_SENSE_HUM, "bme680", 0);
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