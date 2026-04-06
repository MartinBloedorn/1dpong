#include "ConfigWebServer.hpp"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include <sstream>
#include <memory>

ConfigWebServer::ConfigWebServer(const char *ssid, const char *password, int webServerPort)
    : _ssid(ssid), _password(password), _webServerPort(webServerPort), _configCallback(nullptr),
      _webServer(new AsyncWebServer(webServerPort))
{
    _currentConfig = {"192.168.1.1", 8080};
}

ConfigWebServer::~ConfigWebServer()
{
    // Cleanup if needed
}

void ConfigWebServer::begin()
{
    _connectToWiFi();
    _setupWebServer();
}

void ConfigWebServer::setInitialConfig(const ConfigParameters &params)
{
    _currentConfig = params;
}

void ConfigWebServer::setConfigCallback(std::function<void(const ConfigParameters &)> callback)
{
    _configCallback = callback;
}

void ConfigWebServer::_connectToWiFi()
{
    Serial.print("Connecting to WiFi: ");
    Serial.println(_ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("\nFailed to connect to WiFi");
    }
}

void ConfigWebServer::_setupWebServer()
{
    // Serve HTML UI on GET /
    _webServer->on("/", AsyncWebRequestMethod::HTTP_GET, [this](AsyncWebServerRequest *request)
    {
        String html = _generateHTML().c_str();
        request->send(200, "text/html", html);
    });

    // Handle POST request to save config
    _webServer->on("/save", AsyncWebRequestMethod::HTTP_POST, [this](AsyncWebServerRequest *request)
    {
        // Extract parameters from POST request
        if (request->hasParam("ipAddress", true))
        {
            _currentConfig.ipAddress = request->getParam("ipAddress", true)->value().c_str();
        }
        if (request->hasParam("port", true))
        {
            try
            {
                _currentConfig.port = std::stoi(request->getParam("port", true)->value().c_str());
            }
            catch (...)
            {
                Serial.println("Error parsing port number");
            }
        }

        // Call the user-provided callback
        if (_configCallback)
        {
            _configCallback(_currentConfig);
        }

        // Send response
        String response = R"({"status":"success"})";
        request->send(200, "application/json", response);
    });

    _webServer->begin();
    Serial.print("Web server started on port ");
    Serial.println(_webServerPort);
}

std::string ConfigWebServer::_generateHTML() const
{
    std::ostringstream html;
    html << R"(
<!DOCTYPE html>
<html>
<head>
  <title>Device Configuration</title>
  <style>
    body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; }
    .container { background: #f0f0f0; padding: 20px; border-radius: 8px; }
    h1 { color: #333; }
    .form-group { margin: 15px 0; }
    label { display: block; margin-bottom: 5px; font-weight: bold; }
    input { width: 100%; padding: 8px; box-sizing: border-box; }
    button { background: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; }
    button:hover { background: #45a049; }
    .status { margin-top: 20px; padding: 10px; border-radius: 4px; display: none; }
    .success { background: #d4edda; color: #155724; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Device Configuration</h1>
    <form id="configForm">
      <div class="form-group">
        <label for="ipAddress">IP Address:</label>
        <input type="text" id="ipAddress" name="ipAddress" value=")"
         << _currentConfig.ipAddress << R"(" required>
      </div>
      <div class="form-group">
        <label for="port">Port:</label>
        <input type="number" id="port" name="port" value=")"
         << _currentConfig.port << R"(" min="1" max="65535" required>
      </div>
      <button type="submit">Save Configuration</button>
    </form>
    <div id="status" class="status"></div>
  </div>
  
  <script>
    document.getElementById('configForm').addEventListener('submit', function(e) {
      e.preventDefault();
      const formData = new FormData(this);
      
      fetch('/save', {
        method: 'POST',
        body: formData
      })
      .then(response => response.json())
      .then(data => {
        const statusDiv = document.getElementById('status');
        statusDiv.textContent = 'Configuration saved successfully!';
        statusDiv.classList.add('success');
        statusDiv.style.display = 'block';
        setTimeout(() => { statusDiv.style.display = 'none'; }, 3000);
      })
      .catch(error => {
        console.error('Error:', error);
        alert('Failed to save configuration');
      });
    });
  </script>
</body>
</html>
    )";

    return html.str();
}
