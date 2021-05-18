/* WebOTA.ino
 *  
 * by Roland Pelayo 
 * 
 * Update ESP32 firmware via external web server
 */
 
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

// location of firmware file on external web server
// change to your actual .bin location
#define HOST "http://server.com/esp32fw.bin"

HTTPClient client;
// Your WiFi credentials
const char* ssid = "Your WiFi SSID";
const char* password = "Your WiFi Password";
// Global variables
int totalLength;       //total size of firmware
int currentLength = 0; //current size of written firmware

void setup() {
  Serial.begin(9600);
  // Start WiFi connection
  WiFi.mode(WIFI_MODE_STA);        
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  // Connect to external web server
  client.begin(HOST);
  // Get file, just to check if each reachable
  int resp = client.GET();
  Serial.print("Response: ");
  Serial.println(resp);
  // If file is reachable, start downloading
  if(resp > 0){
      // get length of document (is -1 when Server sends no Content-Length header)
      totalLength = client.getSize();
      // transfer to local variable
      int len = totalLength;
      // this is required to start firmware update process
      Update.begin(UPDATE_SIZE_UNKNOWN);
      Serial.printf("FW Size: %u\n",totalLength);
      // create buffer for read
      uint8_t buff[128] = { 0 };
      // get tcp stream
      WiFiClient * stream = client.getStreamPtr();
      // read all data from server
      Serial.println("Updating firmware...");
      while(client.connected() && (len > 0 || len == -1)) {
           // get available data size
           size_t size = stream->available();
           if(size) {
              // read up to 128 byte
              int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
              // pass to function
              updateFirmware(buff, c);
              if(len > 0) {
                 len -= c;
              }
           }
           delay(1);
      }
  }else{
    Serial.println("Cannot download firmware file");
  }
  client.end();
  
}

void loop() {}

// Function to update firmware incrementally
// Buffer is declared to be 128 so chunks of 128 bytes
// from firmware is written to device until server closes
void updateFirmware(uint8_t *data, size_t len){
  Update.write(data, len);
  currentLength += len;
  // Print dots while waiting for update to finish
  Serial.print('.');
  // if current length of written firmware is not equal to total firmware size, repeat
  if(currentLength != totalLength) return;
  Update.end(true);
  Serial.printf("\nUpdate Success, Total Size: %u\nRebooting...\n", currentLength);
  // Restart ESP32 to see changes 
  ESP.restart();
}
