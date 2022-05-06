#include "SimplePortal.h"
static DNSServer _SP_dnsServer;
#ifdef ESP8266
static ESP8266WebServer _SP_server(80);
#else
static WebServer _SP_server(80);
#endif

const char SP_connect_page[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1">
</head><body>
<style type="text/css">
    input[type="text"] {margin-bottom:8px;font-size:20px;}
    input[type="submit"] {width:180px; height:60px;margin-bottom:8px;font-size:20px;}
</style>
<center>
<h3>WiFi settings</h3>
<form action="/connect" method="POST">
    <input type="text" name="ssid" placeholder="SSID" id="ssid">
    <input type="text" name="pass" placeholder="Pass" id="pass">
    <input type="submit" value="Submit">
</form>
    <div id="netlist">Scanning networks...</div>
<h3>Switch WiFi mode</h3>
<form action="/ap" method="POST">
    <input type="submit" value="Access Point">
</form>
<form action="/local" method="POST">
    <input type="submit" value="Local Mode">
</form>
<form action="/exit" method="POST">
    <input type="submit" value="Exit Portal">
</form>
</center>
<script>
function getnetlist() {
  var request = new XMLHttpRequest();
  request.onreadystatechange = function () {
    var DONE = this.DONE || 4;
    if (this.readyState === DONE) {
      var res = this.responseText;
      document.getElementById('netlist').innerHTML = res;
    }
  };
  request.open('POST', '/list', true);
  request.send(null);
}
getnetlist();
timerId = setInterval(() => getnetlist(), 15000);
</script>
</body></html>)rawliteral";

static bool _SP_started = false;
static byte _SP_status = 0;
PortalCfg portalCfg;
String networks = "";

void SP_handleConnect() {
  strcpy(portalCfg.SSID, _SP_server.arg("ssid").c_str());
  strcpy(portalCfg.pass, _SP_server.arg("pass").c_str());
  portalCfg.mode = WIFI_STA;
  _SP_status = 1;
}

void SP_handleList() {
  listNetworks();
  _SP_server.send(200, "text/html", networks );
  showMessage("WiFi Setup", "Submit", "Credentials", 0);
}

void SP_handleAP() {
  portalCfg.mode = WIFI_AP;
  _SP_status = 2;
}

void SP_handleLocal() {
  portalCfg.mode = WIFI_STA;
  _SP_status = 3;
}

void SP_handleExit() {
  _SP_status = 4;
}

void portalStart() {
  WiFi.softAPdisconnect();
  delay(1);
  WiFi.disconnect();
  delay(1);
  IPAddress apIP(SP_AP_IP);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.mode(WIFI_AP);
  delay(1);
  WiFi.softAPConfig(apIP, apIP, subnet);
  delay(1);
  WiFi.softAP(SP_AP_NAME);
  delay(1);
  _SP_dnsServer.start(53, "*", apIP);
  _SP_server.onNotFound([]() {
    listNetworks();
    _SP_server.send(200, "text/html", SP_connect_page);
  });
  _SP_server.on("/connect", HTTP_POST, SP_handleConnect);
  _SP_server.on("/ap", HTTP_POST, SP_handleAP);
  _SP_server.on("/local", HTTP_POST, SP_handleLocal);
  _SP_server.on("/exit", HTTP_POST, SP_handleExit);
  _SP_server.on("/list", HTTP_POST, SP_handleList);
  _SP_server.begin();
  _SP_started = true;
  _SP_status = 0;
}

void portalStop() {
  WiFi.softAPdisconnect();
  _SP_server.stop();
  _SP_dnsServer.stop();
  _SP_started = false;
}

bool portalTick() {
  int remainingTimeBudget = ui.update();
  static bool wifi_conn_old = false;  
  //filemgr.handleClient();
  // Process user input
 // doPushButtons();
  wifi_connected = ( WiFi.softAPgetStationNum() > 0 );
  if (wifi_connected != wifi_conn_old){
    if (wifi_connected) {
      IPAddress apIP(SP_AP_IP);
      showMessage("Connected AP", "Open Site:", "http://" + apIP.toString(), 0);
    } else {
      showMessage("AP Mode", "Connect to AP", "\"" + (String)SP_AP_NAME + "\"", 0);
    }
    wifi_conn_old = wifi_connected;
  }
  refreshUI();
  if (_SP_started) {
    _SP_dnsServer.processNextRequest();
    _SP_server.handleClient();
    
    if (_SP_status) {
      portalStop();
      return 1;
    }
  }
  return 0;
}

void portalRun(uint32_t prd) {
  uint32_t tmr = millis();
  portalStart();
  while (!portalTick()) {
    if (millis() - tmr > prd) {
      _SP_status = 5;
      portalStop();
      break;
    }
  }
}

byte portalStatus() {
  return _SP_status;
}

void listNetworks() {
  // scan for nearby networks:
  networks = "";  
  DEBUG("**WiFi** Scan Networks");
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    DEBUG("**WiFi** Couldn't get a wifi connection");
    while (true);
  }

  // print the list of networks seen:
  DEB("**WiFi** number of available networks:");
  DEBUG(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    DEB(thisNet);
    DEB(") ");
    DEBUG(WiFi.SSID(thisNet));
    networks += "<a href=\"#\" onclick=\"document.getElementById('ssid').value='" + (String)(WiFi.SSID(thisNet)) + "'\">" + (String)(WiFi.SSID(thisNet)) + "</a><br>";
  }
}