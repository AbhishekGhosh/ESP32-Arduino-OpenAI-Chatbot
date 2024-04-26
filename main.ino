// modified from https://github.com/MarcoCiau/esp32_gpt_chatbot
// main part written by https://github.com/MarcoCiau
// edited by repo owner 

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// edit these three lines
const char* ssid = "YOUR-SSID-HERE";
const char* password = "YOUR-HOTSPOT-PASSWORD";
const char* api_key = "OPENAI API KEY HERE";
// stop editing
const char* host = "api.openai.com";
const int httpsPort = 443;
const char ledPin = 2;


WiFiClientSecure httpClient;
/* Function to connect to WiFi */
void connectToWiFi();
/* Function to make HTTP request*/
bool sendHTTPRequest(String prompt, String *result);
/* Function to pass user prompt and send it to OpenAI API*/
String getGptResponse(String prompt, bool parseMsg = true);

void setup()
{
  Serial.begin(115200);
  connectToWiFi();
}

void loop()
{
  if (Serial.available() > 0)
  {
    String prompt = Serial.readStringUntil('\n');
    prompt.trim();
    Serial.print("ESP32 > ");
    Serial.println(prompt);
    String response = getGptResponse(prompt);
    Serial.println("ChatGPT > " + response);
    delay(3000);
  }
  delay(10);
}

void connectToWiFi()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Successful WiFi connection");
}

bool sendHTTPRequest(String prompt, String *result)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("ERROR: Device not connected to WiFi");
    return false;
  }

  // Connect to OpenAI API URL
  httpClient.setInsecure();
  if (!httpClient.connect(host, httpsPort))
  {
    Serial.println("Failed to connect with OpenAI API");
    return false;
  }
  // Build Payload
  String payload = "{\"model\": \"gpt-3.5-turbo\",\"messages\": [{\"role\": \"user\", \"content\": \"" + prompt + "\"}]}";
  Serial.println(payload);

  // Build HTTP Request
  String request = "POST /v1/chat/completions HTTP/1.1\r\n";
  request += "Host: " + String(host) + "\r\n";
  request += "Authorization: Bearer " + String(api_key) + "\r\n";
  request += "Content-Type: application/json\r\n";
  request += "Content-Length: " + String(payload.length()) + "\r\n";
  request += "Connection: close\r\n";
  request += "\r\n" + payload + "\r\n";
  // Send HTTP Request
  httpClient.print(request);
   // LED function
  digitalWrite(ledPin, HIGH);
        delay(1000);
        digitalWrite(ledPin, LOW);
        delay(1000);
 

  // Get Response
  String response = "";
  while (httpClient.connected())
  {
    if (httpClient.available())
    {
      response += httpClient.readStringUntil('\n');
      response += String("\r\n");
    }
  }
  httpClient.stop();

  // Parse HTTP Response Code
  int responseCode = 0;
  if (response.indexOf(" ") != -1)
  {                                                                                                  // If the first space is found
    responseCode = response.substring(response.indexOf(" ") + 1, response.indexOf(" ") + 4).toInt(); // Get the characters following the first space and convert to integer
  }

  if (responseCode != 200)
  {
    Serial.println("The procedure has failed. Info:" + String(response));
    return false;
  }

  // Get JSON Body
  int start = response.indexOf("{");
  int end = response.lastIndexOf("}");
  String jsonBody = response.substring(start, end + 1);

  if (jsonBody.length() > 0)
  {
    *result = jsonBody;
    return true;
  }
  Serial.println("Error: Unable to read the information");
  return false;
}

String getGptResponse(String prompt, bool parseMsg)
{
  String resultStr;
  bool result = sendHTTPRequest(prompt, &resultStr);
  if (!result) return "Error : sendHTTPRequest";
  if (!parseMsg) return resultStr;
  DynamicJsonDocument doc(resultStr.length() + 200);
  DeserializationError error = deserializeJson(doc, resultStr.c_str());
  if (error)
  {
    return "[ERR] deserializeJson() failed: " + String(error.f_str());
  }
  const char *_content = doc["choices"][0]["message"]["content"];
  return String(_content);
}
