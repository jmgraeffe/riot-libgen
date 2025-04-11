var a = 17.27;
var b = 237.7;

function alpha(temp, hum) {
    return (a * temp) / (b + temp) + math.log(hum / 100);
}

var temp_dev = easysaul.reg_find_type_and_name(easysaul.SENSE_TEMP, "bme680");
var hum_dev = easysaul.reg_find_type_and_name(easysaul.SENSE_HUM, "bme680");

if (temp_dev > 0 && hum_dev > 0) {
    var temp = easysaul.reg_read(temp_dev, 0);
    var hum = easysaul.reg_read(hum_dev, 0);
    var dew_point = (b * alpha(temp, hum)) / (a - alpha(temp, hum));

    print("Temperature: " + temp + " °C\nHumidity: " + hum + " %\nDew point: " + dew_point + " °C");
}