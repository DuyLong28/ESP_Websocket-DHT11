#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include "Display.h"

// Khai báo thông số cho DHT11
#define DHTPIN 12 //D6
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Pin điều khiển quạt gió
#define FAN_PIN 14 //D5
bool fanState = false;

#define BTN_FAN 13 //D7

// Thông tin wifi
const char* ssid = "DuyLong";
const char* pass = "12341234";

// Tạo AsyncWebServer ở port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    // Kiểm tra xem thông điệp có hợp lệ không
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        // Kết thúc chuỗi
        data[len] = 0;
        Serial.print("Received message: ");
        for (size_t i = 0; i < len; i++) {
            Serial.print((char)data[i]);  // In từng ký tự của thông điệp
        }
        Serial.println();
      
        // So sánh với thông điệp mong muốn
        if (strcmp((char*)data, "toggleFan") == 0) {
            fanState = !fanState; // Đảo trạng thái quạt
            digitalWrite(FAN_PIN, fanState ? HIGH : LOW); // Bật hoặc tắt quạt
            Serial.println("Quạt đã được bật/tắt!"); // In thông báo để kiểm tra
        } else {
            Serial.println("Lệnh không hợp lệ từ client"); // Thông báo lệnh không hợp lệ
        }
    }
}

// Xử lý sự kiện khi Client kết nối Server
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

// Gửi dữ liệu dưới dạng JSON
void notifyClients(float temperature, float humidity) {
  // Kiểm tra xem nhiệt độ và độ ẩm có hợp lệ hay không
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Lỗi: Không thể đọc dữ liệu từ cảm biến DHT11");
    return;
  }

  // Tạo chuỗi JSON để gửi đến client
  String json = "{\"temperature\":\"" + String(temperature) + "\",\"humidity\":\"" + String(humidity) + "\",\"fanState\":" + (fanState ? "true" : "false") + "}";
  ws.textAll(json); // Gửi dữ liệu cho tất cả client
  Serial.println(json);
}

void setup() {
  Serial.begin(115200);

  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);
  pinMode(BTN_FAN, INPUT_PULLUP);

  // Kiểm tra kết nối OLED
  if (!oled.begin()) {
    Serial.println(F("OLED allocation failed"));
    for (;;); // Dừng chương trình nếu không khởi tạo được màn hình OLED
  }

  // Kết nối wifi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    displayConnectWifi(); // Hiển thị thông tin trên OLED
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  displayConnectedWifi(); // Hiển thị thông tin kết nối WiFi thành công trên OLED

  // Khởi tạo websocket
  initWebSocket();

  // Định tuyến đến trang HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  
  server.begin();
}

void loop() {
  ws.cleanupClients();
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  static float lastTemp = temperature;  // Lưu trữ giá trị nhiệt độ trước đó
  static bool lastButtonState = HIGH;   // Trạng thái trước của nút bấm
  bool buttonState = digitalRead(BTN_FAN);

  unsigned long now = millis();
  static unsigned long lastTime = 0;

  // Cập nhật dữ liệu nhiệt độ và độ ẩm mỗi 3 giây
  if (now - lastTime > 3000) {
    notifyClients(temperature, humidity);
    lastTime = now;
  }

  displayDHT11Values(temperature, humidity);

  // Điều khiển quạt tự động dựa trên thay đổi nhiệt độ
  if (lastTemp <= 35 && temperature > 35 && !fanState) {
    // Nếu nhiệt độ tăng từ ≤ 35 lên > 35, bật quạt
    fanState = true;
    digitalWrite(FAN_PIN, HIGH);
    Serial.println("Quạt đã bật do nhiệt độ tăng từ ≤ 35 lên > 35.");
  } else if (lastTemp > 35 && temperature <= 35 && fanState) {
    // Nếu nhiệt độ giảm từ > 35 xuống ≤ 35, tắt quạt
    fanState = false;
    digitalWrite(FAN_PIN, LOW);
    Serial.println("Quạt đã tắt do nhiệt độ giảm từ > 35 xuống ≤ 35.");
  }

  // Cập nhật giá trị nhiệt độ trước đó
  lastTemp = temperature;

  // Kiểm tra xem nút bấm vật lý đã được nhấn hay chưa
  if (lastButtonState == HIGH && buttonState == LOW) {
    fanState = !fanState; // Đảo trạng thái quạt
    digitalWrite(FAN_PIN, fanState ? HIGH : LOW);
    Serial.println("Quạt đã được bật/tắt từ nút bấm vật lý!");
    notifyClients(temperature, humidity); // Cập nhật trạng thái quạt cho client web
    delay(200); // Chống rung nút bấm
  }

  lastButtonState = buttonState; // Cập nhật trạng thái nút bấm
}
