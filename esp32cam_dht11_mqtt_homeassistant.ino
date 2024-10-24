/////////////////////////////////////////////////////////////////
/*
  Home Assistant with multiple ESP32 Cameras and DHT11 Sensor
  Home Assistant configuration.yml
  ************************************************************
  mqtt:
  camera:
    - topic: esp32/cam_0
  sensor:
    - name: "esp32_Temp"
      state_topic: "esp32/dht_0"
      unit_of_measurement: "°C"
      value_template: "{{ value_json.temperature }}"
      

    - name: "esp32_Hum"
      state_topic: "esp32/dht_0"
      unit_of_measurement: "%"
      value_template: "{{ value_json.humidity }}"
   *************************************************************   
  Created by MOONDORI
*/
/////////////////////////////////////////////////////////////////

/*
- ESP32 Arduino 
- TestDevice: ESP32-CAM AI-Thinker, XIAO ESP32S3 Sense

**Required Library**
- MQTT: 2.5.1
https://github.com/256dpi/arduino-mqtt
*/

#include "esp_camera.h"
#include <WiFiClientSecure.h>
#include <MQTT.h>

#include "DHT.h"

#define DHTPIN 15     // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// DHT(DHT11, DHT22, DHT21, Temperature/Humidity Sensor) setup 
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);



//Select Your Camera Board
//#define CAMERA_MODEL_XIAO_ESP32S3
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

const char ssid[] = "wifi ap ip";
const char pass[] = "wifi ap password";

#define CONFIG_BROKER_URL "mqtt broker ip"

#define CONFIG_BROKER_USERNAME "mqtt user name"
#define CONFIG_BROKER_PASSWORD "mqtt password"

//For the First Camera
#define ESP32CAM_DEVICE "ESP32-CAM-0"
#define ESP32CAM_PUBLISH_TOPIC_CAM "esp32/cam_0"
#define ESP32CAM_PUBLISH_TOPIC_SENSOR "esp32/dht_0"
//For the Second Camera
//#define ESP32CAM_DEVICE "ESP32-CAM-1"
//#define ESP32CAM_PUBLISH_TOPIC "esp32/cam_1"

const int bufferSize = 1024 * 25;  // 25KB

WiFiClient net;
MQTTClient client = MQTTClient(bufferSize);
char data[80];  //DHT MQTT MSG

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.print("\nconnecting...");
  while (!client.connect(ESP32CAM_DEVICE, CONFIG_BROKER_USERNAME, CONFIG_BROKER_PASSWORD)) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");
}

bool camInit() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
 // config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
 // config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
 //     config.grab_mode = CAMERA_GRAB_LATEST;
      config.frame_size = FRAMESIZE_VGA;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
//      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    config.frame_size = FRAMESIZE_VGA;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return false;
  }

  return true;
}

void uploadImage() {
  Serial.println("uploadImage via MQTT");
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("Non-JPEG data not implemented");
    esp_camera_fb_return(fb);
    return;
  }

  if (!client.publish(ESP32CAM_PUBLISH_TOPIC_CAM, (const char*)fb->buf, fb->len)) {
    Serial.println("[Failure] Uploading Image via MQTT");
  }

  esp_camera_fb_return(fb);
}

void setup() {
  Serial.begin(115200);

  dht.begin();
  
  if (!camInit()) {
    Serial.println("[Failure] Cam Init");
    return;
  }
  WiFi.begin(ssid, pass);
  client.begin(CONFIG_BROKER_URL, 1883, net);
  connect();
}

void loop() {
  client.loop();
  delay(10);

  if (!client.connected()) {
    connect();
  } else {
    
    getDHTdata();

    uploadImage();
  }
}


void getDHTdata() {
  delay(2000);
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));


    String dhtReadings = "{\"temperature\":\"" + String(t) + "\", \"humidity\":\"" + String(h) + "\"}";
    dhtReadings.toCharArray(data, (dhtReadings.length() + 1));

  if (!client.publish(ESP32CAM_PUBLISH_TOPIC_SENSOR, data)) {
    Serial.println("[Failure] Uploading Image via MQTT");
  }

}
