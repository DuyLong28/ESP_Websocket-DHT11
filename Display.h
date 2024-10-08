#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// Kết nối OLED
#define SCREEN_ADDRESS 0x3C // SCL D1, SDA D2
Adafruit_SH1106G oled = Adafruit_SH1106G(128, 64, &Wire, -1);

void displayConnectWifi() {
  static int dotCount = 0;  // Biến để theo dõi số lượng dấu chấm
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 10);
  oled.print("Connecting wifi");
  for (int i = 0; i <= dotCount; i++) {
    oled.print(".");
  }
  oled.display();
  dotCount = (dotCount + 1) % 3; // Sau 3 dấu chấm thì quay về 0
  delay(500);
}


void displayConnectedWifi() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0,10);
  oled.println("Wifi Connected");
  oled.println();
  oled.print(WiFi.localIP());
  oled.display();
  delay(3000);
}

void displayDHT11Values(float temperature, float humidity) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 10);
  oled.println("Temperature:");
  oled.setCursor(40, 25);
  oled.print(temperature);
  oled.print(" C");
  oled.setCursor(0, 40);
  oled.println("Humidity:");
  oled.setCursor(40, 55);
  oled.print(humidity);
  oled.print(" %");
  oled.display();
}

// Code web
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Websocket</title>
</head>
<body>
    <div class="container">
        <h1>ESP32 Websocket</h1>
        <div class="data nd">
            <div class="name">Nhiệt độ</div>
            <div class="value" id="temperature">
                <span>°C</span>
            </div>
        </div>
        <div class="data da">
            <div class="name">Độ ẩm</div>
            <div class="value" id="humidity">
                <span>%</span>
            </div>
        </div>
        <div class="data btn">
            <div class="name">Quạt gió</div>
            <button id="fan" onclick="toggleFan()">
                <span id="btnstatus"></span>
            </button>
        </div>
    </div>
</body>
<style>
    html, body {
        margin: 0;
        padding: 0;
        font-family: Arial, sans-serif;
        height: 100%;
    }
    .container {
        text-align: center;
        width: 100%;
        position: absolute;
        display: flex;
        flex-direction: column;
        align-items: center;
    }
    h1 {
        text-align: center;
        font-size: 30px;
    }
    .data {
        text-align: center;
        width: 200px;
        border-radius: 15px;
        padding: 10px;
        margin: 10px;
    }
    .data.nd {
        background-color: #e17979;
    }
    .data.da {
        background-color: #79a6e1;
    }
    .data.btn {
        border: solid 2px black;
    }
    .name, .value {
        font-size: 20px;
    }
    button {
        background-color: #ffffff;
        width: 70%;
        height: 40px;
        border-radius: 20px;
        margin: 10px;
        font-size: 15px;
        border: none;
        transition: background-color 0.3s;
        border: solid 1px black;
    }
    button:hover {
        background-color: #ffffff;
        color: rgb(0, 0, 0);
    }
</style>
<script>
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;
    window.addEventListener('load', onLoad);

    function onLoad(event) {
        initWebSocket();
        document.getElementById('fan').addEventListener('click', toggleFan);
    }

    function initWebSocket() {
        websocket = new WebSocket(gateway);
        websocket.onmessage = onMessage;
    }

    function onMessage(event) {
        var data = JSON.parse(event.data);
        document.getElementById('temperature').innerHTML = data.temperature + " C";
        document.getElementById('humidity').innerHTML = data.humidity + " %";
        
        // Cập nhật trạng thái nút (Bật/Tắt)
        var fanStatus = data.fanState ? "Bật" : "Tắt";
        document.getElementById('btnstatus').innerHTML = fanStatus;

        // Đổi màu sắc nút dựa trên trạng thái quạt
        updateFanButton(data.fanState);
    }

    let isSendingToggleCommand = false;

    function toggleFan() {
        if (!isSendingToggleCommand) {
            isSendingToggleCommand = true; // Đánh dấu là đang gửi lệnh
            websocket.send('toggleFan'); // Gửi lệnh đến server
            setTimeout(() => {
                isSendingToggleCommand = false; // Đánh dấu là đã gửi xong
            }, 500); // Đặt thời gian cho phép gửi lại lệnh
        }
    }
    // Hàm cập nhật màu sắc nút dựa trên trạng thái của quạt
    function updateFanButton(fanState) {
        var button = document.getElementById('fan');
        
        if (fanState) {
            button.style.backgroundColor = "green";  // Màu xanh lá khi quạt bật
            button.style.color = "white";
            document.getElementById('btnstatus').innerHTML = "ON";  // Cập nhật thành ON khi bật
        } else {
            button.style.backgroundColor = "red";    // Màu đỏ khi quạt tắt
            button.style.color = "white";
            document.getElementById('btnstatus').innerHTML = "OFF";  // Cập nhật thành OFF khi tắt
        }
    }
</script>
</html>
)rawliteral";