var autoFlag = false;
var timer;

$(function () {
    function updateSensorData() {
        console.log("update sensor data")
        $.get("/cgi-bin/sensor_data", function(data){
            console.log(data)
            var obj = $.parseJSON(data);
            $("#temp").text(obj["temp"]/10)
            $("#humi").text(obj["humi"]/10)
            $("#pm25").text(obj["pm25"])
            $("#hcho").text(obj["hcho"]/1000)
        });
    }

    $("#update-btn").click(function() {
        autoFlag = !autoFlag;
        if (autoFlag) {
            $(this).text("停止更新")
            console.log("auto")
            timer = setInterval(updateSensorData, 1000)
        }
        else {
            $(this).text("自动更新")
            console.log("disable")
            clearInterval(timer)
        }
    });

    $("input:checkbox").click(function () {
        if ($(this).prop("checked")) {
            var data = '{\"id\":' + $(this).val() + ',\"sw\":\"On\"}'
        }
        else {
            var data = '{\"id\":' + $(this).val() + ',\"sw\":\"Off\"}'
        }
        console.log(data)

        $.post("/cgi-bin/relay_control", data)
    });
});