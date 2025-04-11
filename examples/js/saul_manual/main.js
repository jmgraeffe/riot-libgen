var EASYSAUL_SENSE_TEMP = 130;
var EASYSAUL_SENSE_HUM = 131;

var a = 17.27;
var b = 237.7;

function alpha(temp, hum) {
    return (a * temp) / (b + temp) + math.log(hum / 100);
}

var temp = easysaul.reg_find_and_read(EASYSAUL_SENSE_TEMP, "bme680", 0);
var hum = easysaul.reg_find_and_read(EASYSAUL_SENSE_HUM, "bme680", 0);
var dew_point = (b * alpha(temp, hum)) / (a - alpha(temp, hum));
print("Temperature: " + temp + " °C\nHumidity: " + hum + " %\nDew point: " + dew_point + " °C");