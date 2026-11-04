/*
 * Fake SAUL devices, so that the SAUL registry can be tested without any sensors (on native as well as on boards).
 */
#include "phydat.h"
#include "saul.h"
#include "saul_reg.h"

#include "libgen_tests.h"

static int _read_temp(const void *dev, phydat_t *res) {
    (void) dev;
    res->val[0] = 2150;
    res->val[1] = 0;
    res->val[2] = 0;
    res->unit = UNIT_TEMP_C;
    res->scale = -2;
    return 1;
}

static int _read_hum(const void *dev, phydat_t *res) {
    (void) dev;
    res->val[0] = 4520;
    res->val[1] = 0;
    res->val[2] = 0;
    res->unit = UNIT_PERCENT;
    res->scale = -2;
    return 1;
}

static const saul_driver_t _temp_driver = {
    .read = _read_temp,
    .write = saul_write_notsup,
    .type = SAUL_SENSE_TEMP,
};

static const saul_driver_t _hum_driver = {
    .read = _read_hum,
    .write = saul_write_notsup,
    .type = SAUL_SENSE_HUM,
};

static saul_reg_t _temp = { .name = "fake_temp", .driver = &_temp_driver };
static saul_reg_t _hum = { .name = "fake_hum", .driver = &_hum_driver };

void libgen_tests_saul_init(void) {
    saul_reg_add(&_temp);
    saul_reg_add(&_hum);
}
