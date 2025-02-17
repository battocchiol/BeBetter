#include <WiFi.h>
#include <DNSServer.h>
#include <HTTPClient.h>

const char* apSSID = "BeBetter";
const char* apPassword = "12345678";
const int sensorPinTemperature = A0;

char ssid[32] = "";
char password[64] = "";
char email[64] = "";
float previousTemperature = 0; // Variabile globale per memorizzare l'ultimo valore di temperatura


WiFiServer server(80);
DNSServer dnsServer;

IPAddress apIP(192, 168, 4, 3);

#define SUPABASE_URL "SUPABASE_URL"
#define SUPABASE_KEY "SUPABASE_KEY"

bool isIPConflict(IPAddress ip) {
  WiFiClient testClient;
  testClient.connect(ip, 80);
  return testClient.connected();
}

void setup() {
  Serial.begin(115200);

  for (int pinNumber = 2; pinNumber < 5; pinNumber++){
    pinMode(pinNumber, OUTPUT);
    digitalWrite(pinNumber, LOW);
  }
  delay(1000);
  
  WiFi.mode(WIFI_AP);
  
  while (isIPConflict(apIP)) {
    apIP[3]++;
  }

  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID, apPassword);

  dnsServer.start(53, "*", apIP);
  server.begin();
  
  Serial.println("AP Configurato. In attesa di connessione...");
}

void loop() {
  dnsServer.processNextRequest();
  WiFiClient client = server.available();
  if (client) {
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n') {
          if (request.indexOf("GET /save") >= 0) {
            parseCredentials(request);
            connectToWiFi(client);
          } else {
            sendWiFiConfigPage(client);
          }
          break;
        }
      }
    }
  }
}

void sendWiFiConfigPage(WiFiClient& client) {
  String html = "<html><head><title>BeBetter Wi-Fi Setup</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<script>";
  
  // Funzione per validare email e abilitare il pulsante
  html += "function validateForm() {";
  html += "  var email1 = document.getElementById('email1').value;";
  html += "  var email2 = document.getElementById('email2').value;";
  html += "  var emailPattern = /^[^@\\s]+@[^@\\s]+\\.[^@\\s]+$/;";
  html += "  var checkbox = document.getElementById('confirmCheck');";
  html += "  var submitButton = document.getElementById('submitButton');";
  html += "  var errorText = document.getElementById('errorText');";

  html += "  if (email1 === email2 && emailPattern.test(email1)) {";
  html += "    errorText.innerHTML = '';";
  html += "    submitButton.disabled = !checkbox.checked;";
  html += "  } else {";
  html += "    errorText.innerHTML = '⚠️ Le email non coincidono o sono errate!';";
  html += "    submitButton.disabled = true;";
  html += "  }";
  html += "}";

  html += "</script></head><body style='background-color: #e0f7fa; text-align: center;'>";
  html += "<h1 style='color: #00796b;'>Configurazione Wi-Fi</h1>";
  html += "<form action='/save' method='GET' style='background-color: white; padding: 20px; border-radius: 10px; display: inline-block;'>";
  html += "<label style='color: #00796b;'>SSID</label><br><input type='text' name='ssid' required><br><br>";
  html += "<label style='color: #00796b;'>Password</label><br><input type='password' name='password' required><br><br>";
  html += "<label style='color: #00796b;'>Email</label><br><input type='text' id='email1' name='email' onkeyup='validateForm()' required><br><br>";
  html += "<label style='color: #00796b;'>Conferma Email</label><br><input type='text' id='email2' onkeyup='validateForm()' required><br><br>";

  // Messaggio di errore
  html += "<div id='errorText' style='color: red; font-weight: bold; margin-bottom: 10px;'></div>";

  // Messaggio di avviso
  html += "<div id='warning' style='color: red; font-weight: bold; margin-bottom: 10px;'>";
  html += "Se le credenziali Wi-Fi sono errate, dovrai riconnetterti manualmente alla rete \"BeBetter\".";
  html += "</div>";

  // Checkbox per accettare l'avviso
  html += "<input type='checkbox' id='confirmCheck' onclick='validateForm()'> ";
  html += "<label for='confirmCheck'>Ho capito</label><br><br>";

  // Pulsante di connessione
  html += "<button type='submit' id='submitButton' disabled style='background-color: #00796b; color: white; padding: 10px 20px; border: none; border-radius: 5px;'>Connetti</button>";
  html += "</form>";
  html += "</body></html>";

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println(html);
}


void parseCredentials(String request) {
  int ssidStart = request.indexOf("ssid=") + 5;
  int ssidEnd = request.indexOf("&", ssidStart);
  int passwordStart = request.indexOf("password=") + 9;
  int passwordEnd = request.indexOf("&", passwordStart);
  int emailStart = request.indexOf("email=") + 6;
  int emailEnd = request.indexOf(" ", emailStart);

  if (ssidStart > 4 && ssidEnd > ssidStart) {
    String ssidValue = request.substring(ssidStart, ssidEnd);
    ssidValue.replace("%20", " ");
    strncpy(ssid, ssidValue.c_str(), sizeof(ssid) - 1);
  }
  if (passwordStart > 8 && passwordEnd > passwordStart) {
    String passwordValue = request.substring(passwordStart, passwordEnd);
    passwordValue.replace("%20", " ");
    strncpy(password, passwordValue.c_str(), sizeof(password) - 1);
  }
  if (emailStart > 5 && emailEnd > emailStart) {
    String emailValue = request.substring(emailStart, emailEnd);
    emailValue.replace("%40", "@");
    strncpy(email, emailValue.c_str(), sizeof(email) - 1);
  }
}

void connectToWiFi(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<html><body>");
  client.println("<h1>Connessione in corso...</h1>");
  client.println("<p>Attendere qualche secondo...</p>");
  client.println("<script>setTimeout(function(){ window.location.href = '/'; }, 5000);</script>");
  client.println("</body></html>");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int attempts = 0;
  Serial.print("Connessione al Wi-Fi");

  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Connesso al Wi-Fi!");
    int loop = 1;
   for (int i = 0; i < 100; i++) {
      float temperature = readTemperature();
      Serial.print("Temperature: ");
      Serial.println(temperature);
      
      sendToSupabase(temperature, "Temperature");
      getSumFromSupabase();
      delay(10000);
    }

    
  } else {
    Serial.println("\n❌ Errore di connessione. Riapro la pagina di configurazione...");
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPassword);
  }
}

void sendToSupabase(int dato, String dataType) {
  HTTPClient http;
  String url = String(SUPABASE_URL) + "/rest/v1/sensor_data";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);

  String payload = "{\"email\": \"" + String(email) + "\",  \"dataType\": \"" + String(dataType) + "\", \"data\": " + String(dato) + "}";
  Serial.println(payload);
  int httpCode = http.POST(payload);

  if (httpCode >= 200 && httpCode <= 201) {
    Serial.println("Dati inviati a Supabase!");
  } else {
    Serial.println("Errore durante l'invio dei dati a Supabase. Codice HTTP: " + String(httpCode));
  }
  http.end();
}

void getSumFromSupabase() {
  HTTPClient http;
  String url = String(SUPABASE_URL) + "/rest/v1/rpc/get_avg_by_email?email_param=" + String(email);
  Serial.println(url);
  http.begin(url);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_KEY);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Risposta da Supabase: " + payload);
  } else {
    Serial.println("Errore nel recupero dati da Supabase. Codice HTTP: " + String(httpCode));
  }

  http.end();
}

float readTemperature() {
  int numSamples = 1000;
  float sum = 0;
  
  for (int i = 0; i < numSamples; i++) {
    sum += analogRead(sensorPinTemperature);
    delay(10);
  }
  
  float avgSensorValue = sum / numSamples;
  float voltage = (avgSensorValue / 4095.0) * 5.0;  
  float temperature = (voltage - 0.5) * 100; 
  
  // Controlla se la differenza con la temperatura precedente è maggiore di 2
  if (previousTemperature > 0.0 && abs(temperature - previousTemperature) >= 2) {
    // Se la differenza è maggiore o uguale a 2, rifai il calcolo
    return readTemperature();
  }
  
  previousTemperature = temperature; // Aggiorna il valore precedente
  return temperature;
}