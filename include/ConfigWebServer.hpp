#pragma once

#include <functional>
#include <string>
#include <memory>

// Forward declaration
class AsyncWebServer;

struct ConfigParameters
{
    std::string ipAddress;
    int port;
};

class ConfigWebServer
{
public:
    ConfigWebServer(const char *ssid, const char *password, int webServerPort = 80);
    ~ConfigWebServer();

    void begin();
    void setInitialConfig(const ConfigParameters &params);
    void setConfigCallback(std::function<void(const ConfigParameters &)> callback);

private:
    const char *_ssid;
    const char *_password;
    int _webServerPort;
    ConfigParameters _currentConfig;
    std::function<void(const ConfigParameters &)> _configCallback;
    std::unique_ptr<AsyncWebServer> _webServer;

    void _connectToWiFi();
    void _setupWebServer();
    std::string _generateHTML() const;
};
