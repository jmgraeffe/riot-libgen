#ifdef __cplusplus
extern "C" int printf( const char *, ...);
#define WASM_EXPORT __attribute__((visibility("default"))) extern "C"
#else
extern int printf( const char *, ...);
#define WASM_EXPORT __attribute__((visibility("default")))
#endif

#include <stddef.h>
#include <stdint.h>

#define SAUL_SENSE_TEMP 130
#define SAUL_SENSE_HUM 131

// same layout as phydat_t on the host
typedef struct {
    int16_t val[3];
    uint8_t unit;
    int8_t scale;
} phydat_t;

void io_printd(double d);
void io_printi32(int32_t i);
double math_log(double arg);
// devices are passed as their position in the SAUL registry + 1, so that 0 means "not found"
int64_t saul_reg_find_type_and_name(uint8_t type, const char* name);
int32_t saul_reg_read(int64_t dev, phydat_t* res);

static const double a = 17.27;
static const double b = 237.7;

double alpha(double temp, double hum) {
    return (a * temp) / (b + temp) + math_log(hum / 100);
}

// reads a SAUL device and returns its value (sum of all dimensions) in units of 10^power
// return value INT32_MIN means error while reading value
// return value INT32_MAX means value out of int32_t bounds
static int32_t reg_read(int64_t dev, int8_t power) {
    phydat_t phydat = { 0 };

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
    int64_t temp_dev = saul_reg_find_type_and_name(SAUL_SENSE_TEMP, "bme680");
    int64_t hum_dev = saul_reg_find_type_and_name(SAUL_SENSE_HUM, "bme680");
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