#include <WiFi.h>
#include <HTTPClient.h>

// Wi-Fi credentials
const char* ssid = "iPhone";          // Replace with your Wi-Fi SSID
const char* password = "Alamak323";   // Replace with your Wi-Fi Password

// Pin configuration for the button
const int buttonPin = 12; // GPIO pin connected to the button

// URL to send the request to
const char* serverUrl = "https://test.kaunselingadtectaiping.com.my/test.php";

// Variables to track button state
int buttonState = 0;    // Current button state
int lastButtonState = 0; // Previous button state

void setup() {
  // Start serial communication
  Serial.begin(115200);

  // Connect to Wi-Fi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Waiting for WiFi connection...");
  }
  Serial.println("WiFi connected!");

  // Set up button pin
  pinMode(buttonPin, INPUT_PULLUP);  // Button with internal pull-up resistor
}

void loop() {
  // Read the current state of the button
  buttonState = digitalRead(buttonPin);

  // Check if the button was pressed (HIGH to LOW transition)
  if (buttonState == LOW && lastButtonState == HIGH) {
    Serial.println("Button pressed! Sending HTTP request...");
    
    // Send the HTTP request to the server
    sendHTTPRequest();

    // Add a small delay to debounce the button
    delay(200);
  }

  // Save the button state for the next loop iteration
  lastButtonState = buttonState;

  delay(50);  // Small delay to avoid excessive checking
}

void sendHTTPRequest() {
  // Check WiFi status
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Begin the HTTP request
    http.begin(serverUrl);

    // Send the HTTP GET request (you can change this to POST if needed)
    int httpCode = http.GET();

    // Check the response
    if (httpCode > 0) {
      Serial.printf("HTTP request successful. Code: %d\n", httpCode);
      String payload = http.getString();
      Serial.println("Response: " + payload);
    } else {
      Serial.printf("HTTP request failed. Code: %d\n", httpCode);
    }

    // End the HTTP connection
    http.end();
  } else {
    Serial.println("Error: Not connected to WiFi");
  }
}
