#include "web_server.h"
#include <WiFi.h>

void ConfigWebServer::begin(SettingsManager& settings, WiFiManager& wifi,
                             APIClient& api) {
    _settings = &settings;
    _wifi = &wifi;
    _api = &api;

    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/save", HTTP_POST, [this]() { handleSave(); });
    _server.on("/status", HTTP_GET, [this]() { handleStatus(); });
    _server.on("/reboot", HTTP_POST, [this]() { handleReboot(); });
    _server.on("/reset", HTTP_POST, [this]() { handleReset(); });
    _server.on("/scan", HTTP_GET, [this]() { handleScan(); });

    _server.begin();
    Serial.println("Web server started on port 80");
}

void ConfigWebServer::handleClient() {
    _server.handleClient();
}

void ConfigWebServer::handleRoot() {
    _server.send(200, "text/html", buildPage());
}

void ConfigWebServer::handleSave() {
    if (_server.hasArg("wifi_ssid")) {
        _settings->get().wifiSSID = _server.arg("wifi_ssid");
    }
    if (_server.hasArg("wifi_pass") && _server.arg("wifi_pass").length() > 0) {
        _settings->get().wifiPassword = _server.arg("wifi_pass");
    }
    if (_server.hasArg("srv_host")) {
        _settings->get().serverHost = _server.arg("srv_host");
    }
    if (_server.hasArg("srv_port")) {
        _settings->get().serverPort = _server.arg("srv_port").toInt();
    }
    if (_server.hasArg("api_key")) {
        _settings->get().apiKey = _server.arg("api_key");
    }
    _settings->get().useHTTPS = _server.hasArg("use_https");

    if (_server.hasArg("brightness")) {
        _settings->get().brightness = _server.arg("brightness").toInt();
    }
    if (_server.hasArg("poll_int")) {
        _settings->get().pollInterval = _server.arg("poll_int").toInt();
    }
    if (_server.hasArg("dev_name") && _server.arg("dev_name").length() > 0) {
        _settings->get().deviceName = _server.arg("dev_name");
    }

    _settings->save();

    _server.sendHeader("Location", "/?saved=1");
    _server.send(302, "text/plain", "Settings saved. Redirecting...");
}

void ConfigWebServer::handleStatus() {
    String json = "{";
    json += "\"wifi_state\":" + String(_wifi->getState()) + ",";
    json += "\"wifi_ip\":\"" + _wifi->getIP() + "\",";
    json += "\"api_connected\":" + String(_api->isConnected() ? "true" : "false") + ",";
    json += "\"api_error\":\"" + htmlEscape(_api->getLastError()) + "\",";
    json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"uptime\":" + String(millis() / 1000);
    json += "}";
    _server.send(200, "application/json", json);
}

void ConfigWebServer::handleReboot() {
    _server.send(200, "text/html",
        "<html><body><h2>Rebooting...</h2>"
        "<p>Device will restart in a few seconds.</p>"
        "<script>setTimeout(()=>window.location='/',10000)</script>"
        "</body></html>");
    delay(500);
    ESP.restart();
}

void ConfigWebServer::handleReset() {
    _settings->reset();
    _server.send(200, "text/html",
        "<html><body><h2>Settings Reset</h2>"
        "<p>All settings cleared. Device will restart.</p>"
        "<script>setTimeout(()=>window.location='/',10000)</script>"
        "</body></html>");
    delay(500);
    ESP.restart();
}

void ConfigWebServer::handleScan() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + htmlEscape(WiFi.SSID(i)) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
    }
    json += "]";
    WiFi.scanDelete();
    _server.send(200, "application/json", json);
}

String ConfigWebServer::htmlEscape(const String& str) {
    String out = str;
    out.replace("&", "&amp;");
    out.replace("<", "&lt;");
    out.replace(">", "&gt;");
    out.replace("\"", "&quot;");
    out.replace("'", "&#39;");
    return out;
}

String ConfigWebServer::buildPage() {
    auto& s = _settings->get();

    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>TapeBackarr CYD Setup</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,sans-serif;background:#0a0e1a;color:#e0e0e0;padding:20px;max-width:600px;margin:0 auto}
h1{color:#04ffff;margin-bottom:5px;font-size:1.5em}
.sub{color:#888;margin-bottom:20px;font-size:0.9em}
.card{background:#1a1f2e;border-radius:8px;padding:16px;margin-bottom:16px;border:1px solid #2a2f3e}
.card h2{color:#04ffff;font-size:1.1em;margin-bottom:12px;border-bottom:1px solid #2a2f3e;padding-bottom:8px}
label{display:block;margin-bottom:4px;color:#aaa;font-size:0.85em}
input[type=text],input[type=password],input[type=number]{width:100%;padding:8px 12px;background:#0a0e1a;border:1px solid #3a3f4e;border-radius:4px;color:#e0e0e0;margin-bottom:12px;font-size:0.95em}
input:focus{border-color:#04ffff;outline:none}
.row{display:flex;gap:12px}
.row>div{flex:1}
.checkbox{display:flex;align-items:center;gap:8px;margin-bottom:12px}
.checkbox input{width:auto;margin:0}
.checkbox label{margin:0}
button{padding:10px 20px;border:none;border-radius:4px;cursor:pointer;font-size:0.95em;font-weight:600}
.btn-primary{background:#04ffff;color:#0a0e1a;width:100%}
.btn-primary:hover{background:#00cccc}
.btn-danger{background:#f44;color:#fff;margin-top:8px}
.btn-danger:hover{background:#d33}
.btn-warn{background:#ffaa00;color:#0a0e1a;margin-top:8px}
.btn-warn:hover{background:#ffcc88}
.btn-group{display:flex;gap:8px;margin-top:12px}
.btn-group button{flex:1}
.status{display:flex;gap:16px;flex-wrap:wrap}
.status .item{text-align:center;flex:1;min-width:80px}
.status .val{font-size:1.3em;font-weight:bold;color:#04ffff}
.status .lbl{font-size:0.75em;color:#888}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:4px}
.green{background:#0f0}.red{background:#f00}.orange{background:#ffaa00}
.alert{background:#1a3a1a;border:1px solid #0f0;border-radius:4px;padding:10px;margin-bottom:16px;color:#0f0;text-align:center}
.scan-btn{background:#2a2f3e;color:#04ffff;padding:6px 12px;font-size:0.8em;margin-bottom:8px}
#networks{margin-bottom:12px}
.net-item{padding:6px;background:#0a0e1a;border-radius:4px;margin:4px 0;cursor:pointer;display:flex;justify-content:space-between}
.net-item:hover{background:#1a2030}
</style>
</head>
<body>
<h1>TapeBackarr CYD</h1>
<p class="sub">ESP32 Tape Drive Monitor Setup</p>
)rawliteral";

    if (_server.hasArg("saved")) {
        html += "<div class='alert'>Settings saved! Reboot to apply WiFi changes.</div>";
    }

    // Status card
    html += "<div class='card'><h2>Status</h2><div class='status'>";
    html += "<div class='item'><div class='val'>";
    html += (_wifi->getState() == WIFI_STATE_CONNECTED) ? "<span class='dot green'></span>Connected" : "<span class='dot red'></span>Disconnected";
    html += "</div><div class='lbl'>WiFi</div></div>";
    html += "<div class='item'><div class='val'>" + _wifi->getIP() + "</div><div class='lbl'>IP Address</div></div>";
    html += "<div class='item'><div class='val'>";
    html += _api->isConnected() ? "<span class='dot green'></span>OK" : "<span class='dot red'></span>N/A";
    html += "</div><div class='lbl'>API</div></div>";
    html += "<div class='item'><div class='val'>" + String(ESP.getFreeHeap() / 1024) + " KB</div><div class='lbl'>Free Heap</div></div>";
    html += "</div></div>";

    // WiFi settings
    html += "<form method='POST' action='/save'>";
    html += "<div class='card'><h2>WiFi Settings</h2>";
    html += "<button type='button' class='scan-btn' onclick='scanWiFi()'>Scan Networks</button>";
    html += "<div id='networks'></div>";
    html += "<label>SSID</label>";
    html += "<input type='text' name='wifi_ssid' id='wifi_ssid' value='" + htmlEscape(s.wifiSSID) + "'>";
    html += "<label>Password</label>";
    html += "<input type='password' name='wifi_pass' placeholder='Leave empty to keep current'>";
    html += "</div>";

    // Server settings
    html += "<div class='card'><h2>TapeBackarr Server</h2>";
    html += "<label>Server Host / IP</label>";
    html += "<input type='text' name='srv_host' value='" + htmlEscape(s.serverHost) + "' placeholder='192.168.1.100'>";
    html += "<div class='row'><div>";
    html += "<label>Port</label>";
    html += "<input type='number' name='srv_port' value='" + String(s.serverPort) + "'>";
    html += "</div><div>";
    html += "<label>Poll Interval (s)</label>";
    html += "<input type='number' name='poll_int' value='" + String(s.pollInterval) + "' min='1' max='300'>";
    html += "</div></div>";
    html += "<label>API Key</label>";
    html += "<input type='password' name='api_key' value='" + htmlEscape(s.apiKey) + "' placeholder='Enter your API key'>";
    html += "<div class='checkbox'><input type='checkbox' name='use_https' id='use_https'";
    if (s.useHTTPS) html += " checked";
    html += "><label for='use_https'>Use HTTPS</label></div>";
    html += "</div>";

    // Display settings
    html += "<div class='card'><h2>Display Settings</h2>";
    html += "<div class='row'><div>";
    html += "<label>Brightness (0-100)</label>";
    html += "<input type='number' name='brightness' value='" + String(s.brightness) + "' min='0' max='100'>";
    html += "</div><div>";
    html += "<label>Device Name</label>";
    html += "<input type='text' name='dev_name' value='" + htmlEscape(s.deviceName) + "'>";
    html += "</div></div>";
    html += "</div>";

    // Save button
    html += "<button type='submit' class='btn-primary'>Save Settings</button>";
    html += "</form>";

    // System actions
    html += "<div class='card'><h2>System</h2>";
    html += "<div class='btn-group'>";
    html += "<form method='POST' action='/reboot' style='flex:1'><button type='submit' class='btn-warn' style='width:100%'>Reboot</button></form>";
    html += "<form method='POST' action='/reset' style='flex:1'><button type='submit' class='btn-danger' style='width:100%' onclick=\"return confirm('Reset all settings?')\">Factory Reset</button></form>";
    html += "</div></div>";

    // WiFi scan script
    html += R"rawliteral(
<script>
function scanWiFi(){
  document.getElementById('networks').innerHTML='<p style="color:#888">Scanning...</p>';
  fetch('/scan').then(r=>r.json()).then(nets=>{
    let h='';
    nets.forEach(n=>{
      let bars=n.rssi>-50?'Strong':n.rssi>-70?'Good':'Weak';
      h+='<div class="net-item" onclick="document.getElementById(\'wifi_ssid\').value=\''+n.ssid+'\'"><span>'+n.ssid+(n.secure?' &#x1F512;':'')+'</span><span style="color:#888">'+bars+' ('+n.rssi+')</span></div>';
    });
    if(!nets.length)h='<p style="color:#888">No networks found</p>';
    document.getElementById('networks').innerHTML=h;
  }).catch(()=>{
    document.getElementById('networks').innerHTML='<p style="color:#f44">Scan failed</p>';
  });
}
</script>
)rawliteral";

    html += "</body></html>";
    return html;
}
