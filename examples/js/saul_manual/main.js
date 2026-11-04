var SAUL_SENSE_TEMP = 130;
var SAUL_SENSE_HUM = 131;

var a = 17.27;
var b = 237.7;

function alpha(temp, hum) {
    return (a * temp) / (b + temp) + math.log(hum / 100);
}

// reads a SAUL device and returns its value (sum of all dimensions) in units of 10^power, or undefined on errors
function reg_read(dev, power) {
    var res = {};
    var dim = saul.reg_read(dev, res);
    if (dim < 0) {
        return undefined;
    }

    var value = 0;
    for (var i = 0; i < dim; ++i) {
        value += res.val[i];
    }

    // integer division (truncating towards zero), like in C
    power += res.scale;
    for (; power > 0; --power) {
        value *= 10;
    }
    for (; power < 0; ++power) {
        value = (value - value % 10) / 10;
    }
    return value;
}

var temp_dev = saul.reg_find_type_and_name(SAUL_SENSE_TEMP, "bme680");
var hum_dev = saul.reg_find_type_and_name(SAUL_SENSE_HUM, "bme680");

if (temp_dev > 0 && hum_dev > 0) {
    var temp = reg_read(temp_dev, 0);
    var hum = reg_read(hum_dev, 0);
    var dew_point = (b * alpha(temp, hum)) / (a - alpha(temp, hum));

    print("Temperature: " + temp + " °C\nHumidity: " + hum + " %\nDew point: " + dew_point + " °C");
}