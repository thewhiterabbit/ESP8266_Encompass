/*
  ESP8266_Encompass-Impl.h
  For ESP8266 boards

  ESP8266_Encompass is a library for the ESP8266/Arduino platform, using (ESP)AsyncWebServer to enable easy
  configuration and reconfiguration of WiFi credentials using a Captive Portal.

  Modified from 
  1. Tzapu                https://github.com/tzapu/WiFiManager
  2. Ken Taylor           https://github.com/kentaylor
  3. Alan Steremberg      https://github.com/alanswx/ESPAsyncWiFiManager
  4. Khoi Hoang           https://github.com/khoih-prog/ESPAsync_WiFiManager

  Built by Vague Rabbit   https://github.com/thewhiterabbit/ESP8266_Encompass
  Licensed under MIT license
  Version: 1.3.0

  Version   Modified By   Date          Comments
  -------   -----------   ----------    -----------
  1.0.0     Vague Rabbit  12/7/2020     Initial edits after fork from Khoi Hoang - Removed all ESP32 code/includes
*/

#pragma once///////////////////////////////////

Encompass_DataFields::Encompass_DataFields(const char *custom)
{
  _id = NULL;
  _placeholder = NULL;
  _length = 0;
  _value = NULL;
  _labelPlacement = E_LABEL_BEFORE;

  _customHTML = custom;
}

Encompass_DataFields::Encompass_DataFields(const char *id, const char *placeholder, const char *defaultValue, int length)
{
  init(id, placeholder, defaultValue, length, "", E_LABEL_BEFORE);
}

Encompass_DataFields::Encompass_DataFields(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom)
{
  init(id, placeholder, defaultValue, length, custom, E_LABEL_BEFORE);
}

Encompass_DataFields::Encompass_DataFields(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom, int labelPlacement)
{
  init(id, placeholder, defaultValue, length, custom, labelPlacement);
}

void Encompass_DataFields::init(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom, int labelPlacement)
{
  _id = id;
  _placeholder = placeholder;
  _length = length;
  _labelPlacement = labelPlacement;

  _value = new char[_length + 1];

  if (_value != NULL)
  {
    memset(_value, 0, _length + 1);

    if (defaultValue != NULL)
    {
      strncpy(_value, defaultValue, _length);
    }
  }
  _customHTML = custom;
}

Encompass_DataFields::~Encompass_DataFields()
{
  if (_value != NULL)
  {
    delete[] _value;
  }
}

const char* Encompass_DataFields::getValue()
{
  return _value;
}

const char* Encompass_DataFields::getID()
{
  return _id;
}
const char* Encompass_DataFields::getPlaceholder()
{
  return _placeholder;
}

int Encompass_DataFields::getValueLength()
{
  return _length;
}

int Encompass_DataFields::getLabelPlacement()
{
  return _labelPlacement;
}
const char* Encompass_DataFields::getCustomHTML()
{
  return _customHTML;
}

/**
   [getDataFields description]
   @access public
*/
Encompass_DataFields** ESP8266_Encompass::getDataFields() 
{
  return _DataFields;
}///////////////////////////////////

/**
   [getDataFieldsCount description]
   @access public
*/
int ESP8266_Encompass::getDataFieldsCount() 
{
  return _DataFieldsCount;
}

char* ESP8266_Encompass::getRFC952_hostname(const char* iHostname)
{
  memset(RFC952_hostname, 0, sizeof(RFC952_hostname));

  size_t len = (RFC952_HOSTNAME_MAXLEN < strlen(iHostname)) ? RFC952_HOSTNAME_MAXLEN : strlen(iHostname);

  size_t j = 0;

  for (size_t i = 0; i < len - 1; i++)
  {
    if (isalnum(iHostname[i]) || iHostname[i] == '-')
    {
      RFC952_hostname[j] = iHostname[i];
      j++;
    }
  }
  // no '-' as last char
  if (isalnum(iHostname[len - 1]) || (iHostname[len - 1] != '-'))
    RFC952_hostname[j] = iHostname[len - 1];

  return RFC952_hostname;
}

ESP8266_Encompass::ESP8266_Encompass(AsyncWebServer * webserver, DNSServer *dnsserver, const char *iHostname)
//ESP8266_Encompass::ESP8266_Encompass(const char *iHostname)
{

  server    = webserver;
  dnsServer = dnsserver;
  
  wifiSSIDs     = NULL;
  wifiSSIDscan  = true;
  _modeless     = false;
  shouldscan    = true;
  
#if USE_DYNAMIC_DataFields
  _max_DataFields = WIFI_MANAGER_MAX_DataFields;
  _DataFields = (Encompass_DataFields**)malloc(_max_DataFields * sizeof(Encompass_DataFields*));
#endif

  //WiFi not yet started here, must call WiFi.mode(WIFI_STA) and modify function WiFiGenericClass::mode(wifi_mode_t m) !!!

  WiFi.mode(WIFI_STA);

  if (iHostname[0] == 0)
  {
    String _hostname = "ESP8266-" + String(ESP.getChipId(), HEX);
    _hostname.toUpperCase();
    getRFC952_hostname(_hostname.c_str());
  }
  else
  {
    // Prepare and store the hostname only not NULL
    getRFC952_hostname(iHostname);
  }

  LOGWARN1(F("RFC925 Hostname ="), RFC952_hostname);

  setHostname();

  networkIndices = NULL;
}

ESP8266_Encompass::~ESP8266_Encompass()
{
  if (_DataFields != NULL)
  {
    LOGINFO(F("freeing allocated params!"));

    free(_DataFields);
  }

  if (networkIndices)
  {
    free(networkIndices); //indices array no longer required so free memory
  }
}

bool ESP8266_Encompass::addDataField(Encompass_DataFields *p)
{
  if (_DataFieldsCount == _max_DataFields)
  {
    // rezise the params array
    _max_DataFields += ENCOMPASS_MAX_DATA_FIELDS;
    
    LOGINFO1(F("Increasing _max_DataFields to:"), _max_DataFields);
    
    Encompass_DataFields** new_DataFields = (Encompass_DataFields**)realloc(_DataFields, _max_DataFields * sizeof(Encompass_DataFields*));

    if (new_DataFields != NULL)
    {
      _DataFields = new_DataFields;
    }
    else
    {
      LOGINFO(F("ERROR: failed to realloc params, size not increased!"));
 
      return false;
    }
  }

  _DataFields[_DataFieldsCount] = p;
  _DataFieldsCount++;
  
  LOGINFO1(F("Adding parameter"), p->getID());
  
  return true;
}

void ESP8266_Encompass::setupConfigPortal()
{
  stopConfigPortal = false; //Signal not to close config portal

  /*This library assumes autoconnect is set to 1. It usually is
    but just in case check the setting and turn on autoconnect if it is off.
    Some useful discussion at https://github.com/esp8266/Arduino/issues/1615*/
  if (WiFi.getAutoConnect() == 0)
    WiFi.setAutoConnect(1);

#ifdef ESP8266
  // KH, mod for Async
  server->reset();
#else		//ESP32
  server->reset();
#endif

  /* Setup the DNS server redirecting all the domains to the apIP */
  if (dnsServer)
  {
    dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
  }

  _configPortalStart = millis();

  LOGWARN1(F("\nConfiguring AP SSID ="), _apName);

  if (_apPassword != NULL)
  {
    if (strlen(_apPassword) < 8 || strlen(_apPassword) > 63)
    {
      // fail passphrase to short or long!
      LOGERROR(F("Invalid AccessPoint password. Ignoring"));
      
      _apPassword = NULL;
    }
    LOGWARN1(F("AP PWD ="), _apPassword);
  }
  
  
  // KH, To enable dynamic/random channel
  static int channel;
  // Use random channel if  _WiFiAPChannel == 0
  if (_WiFiAPChannel == 0)
    channel = (_configPortalStart % MAX_WIFI_CHANNEL) + 1;
  else
    channel = _WiFiAPChannel;
  
  if (_apPassword != NULL)
  {
    LOGWARN1(F("AP Channel ="), channel);
    
    //WiFi.softAP(_apName, _apPassword);//password option
    WiFi.softAP(_apName, _apPassword, channel);
  }
  else
  {
    // Can't use channel here
    WiFi.softAP(_apName);
  }
  //////
  
  // Contributed by AlesSt (https://github.com/AlesSt) to solve issue softAP with custom IP sometimes not working
  // See https://github.com/thewhiterabbit/ESP8266_Encompass/issues/26 and https://github.com/espressif/arduino-esp32/issues/985
  // delay 100ms to wait for SYSTEM_EVENT_AP_START
  delay(100);
  //////
  
  //optional soft ip config
  if (_ap_static_ip)
  {
    LOGWARN3(F("Custom AP IP/GW/Subnet = "), _ap_static_ip, _ap_static_gw, _ap_static_sn);
    WiFi.softAPConfig(_ap_static_ip, _ap_static_gw, _ap_static_sn);
  }

  delay(500); // Without delay I've seen the IP address blank
  
  LOGWARN1(F("AP IP address ="), WiFi.softAPIP());

  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  
  server->on("/",         std::bind(&ESP8266_Encompass::handleRoot,         this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
  server->on("/wifi",     std::bind(&ESP8266_Encompass::handleWifi,         this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
  server->on("/save",     std::bind(&ESP8266_Encompass::handleSave,         this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
  server->on("/close",    std::bind(&ESP8266_Encompass::handleServerClose,  this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
  server->on("/i",        std::bind(&ESP8266_Encompass::handleInfo,         this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
  server->on("/r",        std::bind(&ESP8266_Encompass::handleReset,        this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
  server->on("/state",    std::bind(&ESP8266_Encompass::handleState,        this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
  // Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server->on("/fwlink",   std::bind(&ESP8266_Encompass::handleRoot,         this,std::placeholders::_1)).setFilter(ON_AP_FILTER);  
  server->onNotFound (std::bind(&ESP8266_Encompass::handleNotFound,         this, std::placeholders::_1));
  
  server->begin(); // Web server start
  
  LOGWARN(F("HTTP server started"));
}

boolean ESP8266_Encompass::autoConnect()
{
  String ssid = "ESP_" + String(ESP.getChipId());
  return autoConnect(ssid.c_str(), NULL);
}

/* This is not very useful as there has been an assumption that device has to be
  told to connect but Wifi already does it's best to connect in background. Calling this
  method will block until WiFi connects. Sketch can avoid
  blocking call then use (WiFi.status()==WL_CONNECTED) test to see if connected yet.
  See some discussion at https://github.com/tzapu/WiFiManager/issues/68
*/

boolean ESP8266_Encompass::autoConnect(char const *apName, char const *apPassword)
{
#if AUTOCONNECT_NO_INVALIDATE
  LOGINFO(F("\nAutoConnect using previously saved SSID/PW, but keep previous settings"));
  // Connect to previously saved SSID/PW, but keep previous settings
  connectWifi();
#else
  LOGINFO(F("\nAutoConnect using previously saved SSID/PW, but invalidate previous settings"));
  // Connect to previously saved SSID/PW, but invalidate previous settings
  connectWifi(WiFi_SSID(), WiFi_Pass());  
#endif
 
  unsigned long startedAt = millis();

  while (millis() - startedAt < 10000)
  {
    //delay(100);
    delay(200);

    if (WiFi.status() == WL_CONNECTED)
    {
      float waited = (millis() - startedAt);
       
      LOGWARN1(F("Connected after waiting (s) :"), waited / 1000);
      LOGWARN1(F("Local ip ="), WiFi.localIP());
      
      return true;
    }
  }

  return startConfigPortal(apName, apPassword);
}
/////////////////////////
// NEW

String ESP8266_Encompass::networkListAsString()
{
  String pager ;
  
  //display networks in page
  for (int i = 0; i < wifiSSIDCount; i++) 
  {
    if (wifiSSIDs[i].duplicate == true) 
      continue; // skip dups
      
    int quality = getRSSIasQuality(wifiSSIDs[i].RSSI);

    if (_minimumQuality == -1 || _minimumQuality < quality) 
    {
      String item = FPSTR(E_WIFI_LIST_ITEM);
      String rssiQ;
      
      rssiQ += quality;
      item.replace("{v}", wifiSSIDs[i].SSID);
      item.replace("{r}", rssiQ);
      
      if (wifiSSIDs[i].encryptionType != ENC_TYPE_NONE)
      {
        item.replace("{i}", "l");
      } 
      else 
      {
        item.replace("{i}", "");
      }
      
      pager += item;

    } 
    else 
    {
      LOGDEBUG(F("Skipping due to quality"));
    }

  }
  
  return pager;
}

String ESP8266_Encompass::scanModal()
{
  shouldscan = true;
  scan();
  
  String pager = networkListAsString();
  
  return pager;
}

void ESP8266_Encompass::scan()
{
  if (!shouldscan) 
    return;
  
  LOGDEBUG(F("scan: About to scan()"));
  
  if (wifiSSIDscan)
  {
    delay(100);
  }

  if (wifiSSIDscan)
  {
    wifi_ssid_count_t n = WiFi.scanNetworks();
    LOGDEBUG(F("scan: Scan done"));
    
    if (n == WIFI_SCAN_FAILED) 
    {
      LOGDEBUG(F("scan: WIFI_SCAN_FAILED!"));
    }
    else if (n == WIFI_SCAN_RUNNING) 
    {
      LOGDEBUG(F("scan: WIFI_SCAN_RUNNING!"));
    } 
    else if (n < 0) 
    {
      LOGDEBUG(F("scan: Failed with unknown error code!"));
    } 
    else if (n == 0) 
    {
      LOGDEBUG(F("scan: No networks found"));
      // page += F("No networks found. Refresh to scan again.");
    } 
    else 
    {
      if (wifiSSIDscan)
      {
        /* WE SHOULD MOVE THIS IN PLACE ATOMICALLY */
        if (wifiSSIDs) 
          delete [] wifiSSIDs;
          
        wifiSSIDs     = new WiFiResult[n];
        wifiSSIDCount = n;

        if (n > 0)
          shouldscan = false;

        for (wifi_ssid_count_t i = 0; i < n; i++)
        {
          wifiSSIDs[i].duplicate=false;
          bool res=WiFi.getNetworkInfo(i, wifiSSIDs[i].SSID, wifiSSIDs[i].encryptionType, wifiSSIDs[i].RSSI, wifiSSIDs[i].BSSID, wifiSSIDs[i].channel, wifiSSIDs[i].isHidden);
        }

        // RSSI SORT
        // old sort
        for (int i = 0; i < n; i++) 
        {
          for (int j = i + 1; j < n; j++) 
          {
            if (wifiSSIDs[j].RSSI > wifiSSIDs[i].RSSI) 
            {
              std::swap(wifiSSIDs[i], wifiSSIDs[j]);
            }
          }
        }

        // remove duplicates ( must be RSSI sorted )
        if (_removeDuplicateAPs) 
        {
        String cssid;
        
        for (int i = 0; i < n; i++) 
        {
          if (wifiSSIDs[i].duplicate == true) 
            continue;
            
          cssid = wifiSSIDs[i].SSID;
          
          for (int j = i + 1; j < n; j++) 
          {
            if (cssid == wifiSSIDs[j].SSID) 
            {
              LOGDEBUG("scan: DUP AP: " +wifiSSIDs[j].SSID);
              // set dup aps to NULL
              wifiSSIDs[j].duplicate = true; 
            }
          }
        }
        }
      }
    }
  }
}

void ESP8266_Encompass::startConfigPortalModeless(char const *apName, char const *apPassword) 
{
  _modeless     = true;
  _apName       = apName;
  _apPassword   = apPassword;

  WiFi.mode(WIFI_AP_STA);
  
  LOGDEBUG("SET AP STA");

  // try to connect
  if (connectWifi("", "") == WL_CONNECTED)   
  {
    LOGDEBUG1(F("IP Address:"), WiFi.localIP());
       
 	  if ( _savecallback != NULL) 
	  {
	    //todo: check if any custom parameters actually exist, and check if they really changed maybe
	    _savecallback();
	  }
  }

  if ( _apcallback != NULL) 
  {
    _apcallback(this);
  }

  connect = false;
  setupConfigPortal();
  scannow = -1 ;
}

void ESP8266_Encompass::loop()
{
	safeLoop();
	criticalLoop();
}

void ESP8266_Encompass::setInfo() 
{
  if (needInfo) 
  {
    pager       = infoAsString();
    wifiStatus  = WiFi.status();
    needInfo    = false;
  }
}

// Anything that accesses WiFi, ESP or EEPROM goes here

void ESP8266_Encompass::criticalLoop()
{
  LOGDEBUG(F("criticalLoop: Enter"));
  
  if (_modeless)
  {
    if (scannow == -1 || millis() > scannow + TIME_BETWEEN_MODELESS_SCANS)
    {
      LOGDEBUG(F("criticalLoop: modeless scan"));
      
      scan();
      scannow = millis();
    }
    
    if (connect) 
    {
      connect = false;

      LOGDEBUG(F("criticalLoop: Connecting to new AP"));

      // using user-provided  _ssid, _pass in place of system-stored ssid and pass
      //////
      if (connectWifi(_ssid, _pass) != WL_CONNECTED) 
      {
        LOGDEBUG(F("criticalLoop: Failed to connect."));
      } 
      else 
      {
        //connected
        // alanswx - should we have a config to decide if we should shut down AP?
        // WiFi.mode(WIFI_STA);
        //notify that configuration has changed and any optional parameters should be saved
        if ( _savecallback != NULL) 
        {
          //todo: check if any custom parameters actually exist, and check if they really changed maybe
          _savecallback();
        }

        return;
      }

      if (_shouldBreakAfterConfig) 
      {
        //flag set to exit after config after trying to connect
        //notify that configuration has changed and any optional parameters should be saved
        if ( _savecallback != NULL) 
        {
          //todo: check if any custom parameters actually exist, and check if they really changed maybe
          _savecallback();
        }
      }
    }
  }
}

// Anything that doesn't access WiFi, ESP or EEPROM can go here

void ESP8266_Encompass::safeLoop()
{
  #ifndef USE_EADNS	
  dnsServer->processNextRequest();
  #endif
}/////////////////

boolean  ESP8266_Encompass::startConfigPortal()
{
  String ssid = "ESP_" + String(ESP.getChipId());
  ssid.toUpperCase();
  return startConfigPortal(ssid.c_str(), NULL);
}

boolean  ESP8266_Encompass::startConfigPortal(char const *apName, char const *apPassword)
{
  //setup AP
  int connRes = WiFi.waitForConnectResult();

  LOGINFO("WiFi.waitForConnectResult Done");

  if (connRes == WL_CONNECTED)
  {
    LOGINFO("SET AP_STA");
    
    WiFi.mode(WIFI_AP_STA); //Dual mode works fine if it is connected to WiFi
  }
  else
  {
    LOGINFO("SET AP");

    WiFi.mode(WIFI_AP); // Dual mode becomes flaky if not connected to a WiFi network.
    // When ESP8266 station is trying to find a target AP, it will scan on every channel,
    // that means ESP8266 station is changing its channel to scan. This makes the channel of ESP8266 softAP keep changing too..
    // So the connection may break. From http://bbs.espressif.com/viewtopic.php?t=671#p2531
  }

  _apName = apName;
  _apPassword = apPassword;

  //notify we entered AP mode
  if (_apcallback != NULL)
  {
    LOGINFO("_apcallback");
    
    _apcallback(this);
  }

  connect = false;

  setupConfigPortal();

  bool TimedOut = true;

  LOGINFO("ESP8266_Encompass::startConfigPortal : Enter loop");
  
  scannow = -1 ;

  while (_configPortalTimeout == 0 || millis() < _configPortalStart + _configPortalTimeout)
  {
    //DNS
    dnsServer->processNextRequest();
    //HTTP
    //server->handleClient();
    
    //
    //  we should do a scan every so often here and
    //  try to reconnect to AP while we are at it
    //
    if ( scannow == -1 || millis() > scannow + TIME_BETWEEN_MODAL_SCANS)
    {
      LOGDEBUG(F("startConfigPortal: About to modal scan()"));
      
      // since we are modal, we can scan every time
      shouldscan = true;
      
#if defined(ESP8266)
      // we might still be connecting, so that has to stop for scanning
      ETS_UART_INTR_DISABLE ();
      wifi_station_disconnect ();
      ETS_UART_INTR_ENABLE ();
#else
      WiFi.disconnect (false);
#endif

      scan();
      
      //if (_tryConnectDuringConfigPortal) 
      //  WiFi.begin(); // try to reconnect to AP
        
      scannow = millis() ;
    }

    if (connect)
    {
      TimedOut = false;
      delay(2000);

      LOGERROR(F("Connecting to new AP"));

#if 0

      // New Mod from v1.1.0
      int wifiConnected = reconnectWifi();
          
      if ( wifiConnected == WL_CONNECTED )
      {
        //notify that configuration has changed and any optional parameters should be saved
        if (_savecallback != NULL)
        {
          //todo: check if any custom parameters actually exist, and check if they really changed maybe
          _savecallback();
        }
        
        break;
      }
      
      WiFi.mode(WIFI_AP); // Dual mode becomes flaky if not connected to a WiFi network.
      //////

#else

      // using user-provided  _ssid, _pass in place of system-stored ssid and pass
      //////
      if (connectWifi(_ssid, _pass) != WL_CONNECTED)
      {  
        LOGERROR(F("Failed to connect"));
    
        WiFi.mode(WIFI_AP); // Dual mode becomes flaky if not connected to a WiFi network.
      }
      else
      {
        //notify that configuration has changed and any optional parameters should be saved
        if (_savecallback != NULL)
        {
          //todo: check if any custom parameters actually exist, and check if they really changed maybe
          _savecallback();
        }
        break;
      }
      
#endif

      if (_shouldBreakAfterConfig)
      {
        //flag set to exit after config after trying to connect
        //notify that configuration has changed and any optional parameters should be saved
        if (_savecallback != NULL)
        {
          //todo: check if any custom parameters actually exist, and check if they really changed maybe
          _savecallback();
        }
        break;
      }
    }

    if (stopConfigPortal)
    {
      LOGERROR("Stop ConfigPortal");
     
      stopConfigPortal = false;
      break;
    }
    
    yield();
  }

  WiFi.mode(WIFI_STA);
  if (TimedOut)
  {
    setHostname();

    // New v1.0.8 to fix static IP when CP not entered or timed-out
    setWifiStaticIP();
    
    WiFi.begin();
    int connRes = waitForConnectResult();

    LOGERROR1("Timed out connection result:", getStatus(connRes));
  }

  server->reset();
  *dnsServer = DNSServer();

  return  WiFi.status() == WL_CONNECTED;
}

void ESP8266_Encompass::setWifiStaticIP(void)
{ 
#if USE_CONFIGURABLE_DNS
  if (_sta_static_ip)
  {
    LOGWARN(F("Custom STA IP/GW/Subnet"));
   
    //***** Added section for DNS config option *****
    if (_sta_static_dns1 && _sta_static_dns2) 
    { 
      LOGWARN(F("DNS1 and DNS2 set"));
 
      WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn, _sta_static_dns1, _sta_static_dns2);
    }
    else if (_sta_static_dns1) 
    {
      LOGWARN(F("Only DNS1 set"));
     
      WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn, _sta_static_dns1);
    }
    else 
    {
      LOGWARN(F("No DNS server set"));
  
      WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn);
    }
    //***** End added section for DNS config option *****

    LOGINFO1(F("setWifiStaticIP IP ="), WiFi.localIP());
  }
  else
  {
    LOGWARN(F("Can't use Custom STA IP/GW/Subnet"));
  }
#else
  // check if we've got static_ip settings, if we do, use those.
  if (_sta_static_ip)
  {
    WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn);
    
    LOGWARN1(F("Custom STA IP/GW/Subnet : "), WiFi.localIP());
  }
#endif
}

// New from v1.1.1
int ESP8266_Encompass::reconnectWifi(void)
{
  int connectResult;
  
  // using user-provided  _ssid, _pass in place of system-stored ssid and pass
  if ( ( connectResult = connectWifi(_ssid[0], _pass[0]) ) != WL_CONNECTED)
  {  
    LOGERROR1(F("Failed to connect to"), _ssid[0]);
    
    if ( ( connectResult = connectWifi(_ssid[1], _pass[1]) ) != WL_CONNECTED)
    {  
      LOGERROR1(F("Failed to connect to"), _ssid[1]);

    }
    else
      LOGERROR1(F("Connected to"), _ssid[1]);
  }
  else
      LOGERROR1(F("Connected to"), _ssid[0]);
  
  return connectResult;
}

int ESP8266_Encompass::connectWifi(String ssid, String pass)
{
  // Add option if didn't input/update SSID/PW => Use the previous saved Credentials. \
  // But update the Static/DHCP options if changed.
  if ( (ssid != "") || ( (ssid == "") && (WiFi_SSID() != "") ) )
  {  
    //fix for auto connect racing issue. Move up from v1.1.0 to avoid resetSettings()
    if (WiFi.status() == WL_CONNECTED)
    {
      LOGWARN(F("Already connected. Bailing out."));
      return WL_CONNECTED;
    }
     
    if (ssid != "")
      resetSettings();

    setWifiStaticIP();

    WiFi.mode(WIFI_AP_STA); //It will start in station mode if it was previously in AP mode.

    setHostname();

    if (ssid != "")
    {
      // Start Wifi with new values.
      LOGWARN(F("Connect to new WiFi using new IP parameters"));
      
      WiFi.begin(ssid.c_str(), pass.c_str());
    }
    else
    {
      // Start Wifi with old values.
      LOGWARN(F("Connect to previous WiFi using new IP parameters"));
      
      WiFi.begin();
    }
  }
  else if (WiFi_SSID() == "")
  {
    LOGWARN(F("No saved credentials"));
  }

  int connRes = waitForConnectResult();
  LOGWARN1("Connection result: ", getStatus(connRes));

  //not connected, WPS enabled, no pass - first attempt
  if (_tryWPS && connRes != WL_CONNECTED && pass == "")
  {
    startWPS();
    //should be connected at the end of WPS
    connRes = waitForConnectResult();
  }

  return connRes;
}

wl_status_t ESP8266_Encompass::waitForConnectResult()
{
  if (_connectTimeout == 0)
  {
    unsigned long startedAt = millis();
    
    // In ESP8266, WiFi.waitForConnectResult() @return wl_status_t (0-255) or -1 on timeout !!!
    // So, using int for connRes to be safe
    //int connRes = WiFi.waitForConnectResult();
    WiFi.waitForConnectResult();
    
    float waited = (millis() - startedAt);

    LOGWARN1(F("Connected after waiting (s) :"), waited / 1000);
    LOGWARN1(F("Local ip ="), WiFi.localIP());

    // Fix bug from v1.1.0+, connRes is sometimes not correct.
    //return connRes;
    return WiFi.status();
  }
  else
  {
    LOGERROR(F("Waiting WiFi connection with time out"));
    unsigned long start = millis();
    boolean keepConnecting = true;
    
    wl_status_t status;

    while (keepConnecting)
    {
      status = WiFi.status();
      if (millis() > start + _connectTimeout)
      {
        keepConnecting = false;
        LOGERROR(F("Connection timed out"));
      }

      if (status == WL_CONNECTED || status == WL_CONNECT_FAILED)
      {
        keepConnecting = false;
      }
      delay(100);
    }
    
    return status;
  }
}

void ESP8266_Encompass::startWPS()
{
  LOGINFO("START WPS");
  WiFi.beginWPSConfig();
  LOGINFO("END WPS");
}

//Convenient for debugging but wasteful of program space.
//Remove if short of space
const char* ESP8266_Encompass::getStatus(int status)
{
  switch (status)
  {
    case WL_IDLE_STATUS:
      return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL:
      return "WL_NO_SSID_AVAIL";
    case WL_CONNECTED:
      return "WL_CONNECTED";
    case WL_CONNECT_FAILED:
      return "WL_CONNECT_FAILED";
    case WL_DISCONNECTED:
      return "WL_DISCONNECTED";
    default:
      return "UNKNOWN";
  }
}

String ESP8266_Encompass::getConfigPortalSSID()
{
  return _apName;
}

String ESP8266_Encompass::getConfigPortalPW()
{
  return _apPassword;
}

void ESP8266_Encompass::resetSettings()
{
  LOGINFO(F("Previous settings invalidated"));
  WiFi.disconnect(true);
  delay(200);
  return;
}

void ESP8266_Encompass::setTimeout(unsigned long seconds)
{
  setConfigPortalTimeout(seconds);
}

void ESP8266_Encompass::setConfigPortalTimeout(unsigned long seconds)
{
  _configPortalTimeout = seconds * 1000;
}

void ESP8266_Encompass::setConnectTimeout(unsigned long seconds)
{
  _connectTimeout = seconds * 1000;
}

void ESP8266_Encompass::setDebugOutput(boolean debug)
{
  _debug = debug;
}

// KH, To enable dynamic/random channel
int ESP8266_Encompass::setConfigPortalChannel(int channel)
{
  // If channel < MIN_WIFI_CHANNEL - 1 or channel > MAX_WIFI_CHANNEL => channel = 1
  // If channel == 0 => will use random channel from MIN_WIFI_CHANNEL to MAX_WIFI_CHANNEL
  // If (MIN_WIFI_CHANNEL <= channel <= MAX_WIFI_CHANNEL) => use it
  if ( (channel < MIN_WIFI_CHANNEL - 1) || (channel > MAX_WIFI_CHANNEL) )
    _WiFiAPChannel = 1;
  else if ( (channel >= MIN_WIFI_CHANNEL - 1) && (channel <= MAX_WIFI_CHANNEL) )
    _WiFiAPChannel = channel;

  return _WiFiAPChannel;
}

void ESP8266_Encompass::setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn)
{
  LOGINFO(F("setAPStaticIPConfig"));
  _ap_static_ip = ip;
  _ap_static_gw = gw;
  _ap_static_sn = sn;
}

void ESP8266_Encompass::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn)
{
  LOGINFO(F("setSTAStaticIPConfig"));
  _sta_static_ip = ip;
  _sta_static_gw = gw;
  _sta_static_sn = sn;
}

#if USE_CONFIGURABLE_DNS
void ESP8266_Encompass::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn, IPAddress dns_address_1, IPAddress dns_address_2)
{
  LOGINFO(F("setSTAStaticIPConfig for USE_CONFIGURABLE_DNS"));
  _sta_static_ip = ip;
  _sta_static_gw = gw;
  _sta_static_sn = sn;
  _sta_static_dns1 = dns_address_1; //***** Added argument *****
  _sta_static_dns2 = dns_address_2; //***** Added argument *****
}
#endif

void ESP8266_Encompass::setMinimumSignalQuality(int quality)
{
  _minimumQuality = quality;
}

void ESP8266_Encompass::setBreakAfterConfig(boolean shouldBreak)
{
  _shouldBreakAfterConfig = shouldBreak;
}

void ESP8266_Encompass::reportStatus(String &page)
{
  page += FPSTR(E_HTML_SCRIPT_NTP_MSG);

  if (WiFi_SSID() != "")
  {
    page += F("Configured to connect to AP <b>");
    page += WiFi_SSID();

    if (WiFi.status() == WL_CONNECTED)
    {
      page += F(" and connected</b> on IP <a href=\"http://");
      page += WiFi.localIP().toString();
      page += F("/\">");
      page += WiFi.localIP().toString();
      page += F("</a>");
    }
    else
    {
      page += F(" but not connected.</b>");
    }
  }
  else
  {
    page += F("No network configured.");
  }
}

// Handle root or redirect to captive portal
void ESP8266_Encompass::handleRoot(AsyncWebServerRequest *request)
{
  LOGDEBUG(F("Handle root"));

  // Disable _configPortalTimeout when someone accessing Portal to give some time to config
  _configPortalTimeout = 0;

  if (captivePortal(request))
  {
    // If caprive portal redirect instead of displaying the error page.
    return;
  }

  String page = FPSTR(E_HTML_HEAD_START);
  page.replace("{v}", "Options");
  page += FPSTR(E_HTML_SCRIPT);
  page += FPSTR(E_HTML_SCRIPT_NTP);
  page += FPSTR(E_HTML_STYLE);
  page += _customHeadElement;
  page += FPSTR(E_HTML_HEAD_END);
  page += "<h2>";
  page += _apName;

  if (WiFi_SSID() != "")
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      page += " on ";
      page += WiFi_SSID();
    }
    else
    {
      page += " <s>on ";
      page += WiFi_SSID();
      page += "</s>";
    }
  }

  page += "</h2>";
  
  page += FPSTR(E_FLDSET_START);
  
  page += FPSTR(E_HTML_PORTAL);
  page += F("<div class=\"msg\">");
  reportStatus(page);
  page += F("</div>");
  
  page += FPSTR(E_FLDSET_END);
    
  page += FPSTR(E_HTML_END);
 
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", page);
  response->addHeader(FPSTR(E_HTTP_CACHE_CONTROL), FPSTR(E_HTTP_NO_STORE));
  
#if USING_CORS_FEATURE
  // New from v1.1.0, for configure CORS Header, default to E_HTTP_CORS_ALLOW_ALL = "*"
  response->addHeader(FPSTR(E_HTTP_CORS), _CORS_Header);
#endif
  
  response->addHeader(FPSTR(E_HTTP_PRAGMA), FPSTR(E_HTTP_NO_CACHE));
  response->addHeader(FPSTR(E_HTTP_EXPIRES), "-1");
  
  request->send(response);
}

// Wifi config page handler
void ESP8266_Encompass::handleWifi(AsyncWebServerRequest *request)
{
  LOGDEBUG(F("Handle WiFi"));

  // Disable _configPortalTimeout when someone accessing Portal to give some time to config
  _configPortalTimeout = 0;
   
  String page = FPSTR(E_HTML_HEAD_START);
  page.replace("{v}", "Config ESP");
  page += FPSTR(E_HTML_SCRIPT);
  page += FPSTR(E_HTML_SCRIPT_NTP);
  page += FPSTR(E_HTML_STYLE);
  page += _customHeadElement;
  page += FPSTR(E_HTML_HEAD_END);
  page += F("<h2>Configuration</h2>");

  wifiSSIDscan = false;
  LOGDEBUG(F("handleWifi: Scan done"));

  if (wifiSSIDCount == 0) 
  {
    LOGDEBUG(F("handleWifi: No networks found"));
    page += F("No networks found. Refresh to scan again.");
  } 
  else 
  {
    page += FPSTR(E_FLDSET_START);
    
    //display networks in page
    String pager = networkListAsString();
    
    page += pager;
    
    page += FPSTR(E_FLDSET_END);
   
    page += "<br/>";
  }
  
  wifiSSIDscan = true;
  
  page += "<small>To reuse already connected AP, leave SSID & password fields empty</small>";
  
  page += FPSTR(E_HTML_FORM_START);
  char parLength[2];
  
  page += FPSTR(E_FLDSET_START);

  //
  //
  // This needs a lot of editing
  //
  // add the extra parameters to the form
  //////
  for (int i = 0; i < _DataFieldsCount; i++)
  {
    if (_DataFields[i] == NULL)
    {
      break;
    }
    
    String dField;
    
    switch (_DataFields[i]->getLabelPlacement())
    {
      case E_LABEL_BEFORE:
        dField = FPSTR(E_HTML_FORM_LABEL_BEFORE);
        break;
      case E_LABEL_AFTER:
        dField = FPSTR(E_HTML_FORM_LABEL_AFTER);
        break;
      default:
        // E_NO_LABEL
        dField = FPSTR(E_HTML_FORM_FIELD);
        break;
    }

    if (_DataFields[i]->getID() != NULL)
    {
      dField.replace("{i}", _DataFields[i]->getID());
      dField.replace("{n}", _DataFields[i]->getName());
      dField.replace("{p}", _DataFields[i]->getPlaceholder());
      snprintf(parLength, 2, "%d", _DataFields[i]->getValueLength());
      dField.replace("{l}", parLength);
      dField.replace("{v}", _DataFields[i]->getValue());
      dField.replace("{c}", _DataFields[i]->getCustomHTML());
    }
    else
    {
      dField = _DataFields[i]->getCustomHTML();
    }

    page += dField;
  }
  //
  //
  //
  //
  //

  if (_DataFieldsCount > 0)
  {
    page += FPSTR(E_FLDSET_END);
  }

  if (_DataFields[0] != NULL)
  {
    page += "<br/>";
  }

  LOGDEBUG1(F("Static IP ="), _sta_static_ip.toString());
  
  // KH, Comment out to permit changing from DHCP to static IP, or vice versa
  // and add staticIP label in CP
  
  // To permit disable/enable StaticIP configuration in Config Portal from sketch. Valid only if DHCP is used.
  // You'll loose the feature of dynamically changing from DHCP to static IP, or vice versa
  // You have to explicitly specify false to disable the feature.

#if !USE_STATIC_IP_CONFIG_IN_CP
  if (_sta_static_ip)
#endif  
  {
    page += FPSTR(E_FLDSET_START);
    
    String item = FPSTR(E_HTML_FORM_LABEL);
    item += FPSTR(E_HTML_FORM_FIELD);
    item.replace("{i}", "ip");
    item.replace("{n}", "ip");
    item.replace("{p}", "Static IP");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_ip.toString());

    page += item;

    item = FPSTR(E_HTML_FORM_LABEL);
    item += FPSTR(E_HTML_FORM_FIELD);
    item.replace("{i}", "gw");
    item.replace("{n}", "gw");
    item.replace("{p}", "Gateway IP");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_gw.toString());

    page += item;

    item = FPSTR(E_HTML_FORM_LABEL);
    item += FPSTR(E_HTML_FORM_FIELD);
    item.replace("{i}", "sn");
    item.replace("{n}", "sn");
    item.replace("{p}", "Subnet");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_sn.toString());

  #if USE_CONFIGURABLE_DNS
    //***** Added for DNS address options *****
    page += item;

    item = FPSTR(E_HTML_FORM_LABEL);
    item += FPSTR(E_HTML_FORM_FIELD);
    item.replace("{i}", "dns1");
    item.replace("{n}", "dns1");
    item.replace("{p}", "DNS1 IP");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_dns1.toString());

    page += item;

    item = FPSTR(E_HTML_FORM_LABEL);
    item += FPSTR(E_HTML_FORM_FIELD);
    item.replace("{i}", "dns2");
    item.replace("{n}", "dns2");
    item.replace("{p}", "DNS2 IP");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_dns2.toString());
    //***** End added for DNS address options *****
  #endif

    page += item;
    
    page += FPSTR(E_FLDSET_END);

    page += "<br/>";
  }

  page += FPSTR(E_HTML_FORM_END);

  page += FPSTR(E_HTML_END);

  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", page);
  response->addHeader(FPSTR(E_HTTP_CACHE_CONTROL), FPSTR(E_HTTP_NO_STORE));
  
#if USING_CORS_FEATURE
  // New from v1.1.0, for configure CORS Header, default to E_HTTP_CORS_ALLOW_ALL = "*"
  response->addHeader(FPSTR(E_HTTP_CORS), _CORS_Header);
#endif
  
  response->addHeader(FPSTR(E_HTTP_PRAGMA), FPSTR(E_HTTP_NO_CACHE));
  response->addHeader(FPSTR(E_HTTP_EXPIRES), "-1");
  
  request->send(response);

  LOGDEBUG(F("Sent config page"));
}

// Handle the WLAN save form and redirect to WLAN config page again
void ESP8266_Encompass::handleSave(AsyncWebServerRequest *request)
{
  LOGDEBUG(F("WiFi save"));

  //SAVE access points here
  //
  //
  //
  //////
  
  for (int i = 0; i < 2; i++)
  {
    _ssid[i] = request->arg("s").c_str();
    _pass[i] = request->arg("p").c_str();
  }
  //
  //
  //
  //
  //

  //
  //
  // DataFields
  //////
  for (int i = 0; i < _DataFieldsCount; i++)
  {
    if (_DataFields[i] == NULL)
    {
      break;
    }

    //read parameter
    String value = request->arg(_DataFields[i]->getID()).c_str();
    
    //store it in array
    value.toCharArray(_DataFields[i]->_value, _DataFields[i]->_length);
    
    LOGDEBUG2(F("Parameter and value :"), _DataFields[i]->getID(), value);
  }

  if (request->hasArg("ip"))
  {
    String ip = request->arg("ip");
    optionalIPFromString(&_sta_static_ip, ip.c_str());
    
    LOGDEBUG1(F("New Static IP ="), _sta_static_ip.toString());
  }

  if (request->hasArg("gw"))
  {
    String gw = request->arg("gw");
    optionalIPFromString(&_sta_static_gw, gw.c_str());
    
    LOGDEBUG1(F("New Static Gateway ="), _sta_static_gw.toString());
  }

  if (request->hasArg("sn"))
  {
    String sn = request->arg("sn");
    optionalIPFromString(&_sta_static_sn, sn.c_str());
    
    LOGDEBUG1(F("New Static Netmask ="), _sta_static_sn.toString());
  }

#if USE_CONFIGURABLE_DNS
  //*****  Added for DNS Options *****
  if (request->hasArg("dns1"))
  {
    String dns1 = request->arg("dns1");
    optionalIPFromString(&_sta_static_dns1, dns1.c_str());
    
    LOGDEBUG1(F("New Static DNS1 ="), _sta_static_dns1.toString());
  }

  if (request->hasArg("dns2"))
  {
    String dns2 = request->arg("dns2");
    optionalIPFromString(&_sta_static_dns2, dns2.c_str());
    
    LOGDEBUG1(F("New Static DNS2 ="), _sta_static_dns2.toString());
  }
  //*****  End added for DNS Options *****
#endif

  String page = FPSTR(E_HTML_HEAD_START);
  page.replace("{v}", "Credentials Saved");
  page += FPSTR(E_HTML_SCRIPT);
  page += FPSTR(E_HTML_SCRIPT_NTP);
  page += FPSTR(E_HTML_STYLE);
  page += _customHeadElement;
  page += FPSTR(E_HTML_HEAD_END);
  page += FPSTR(E_HTML_SAVED);
  page.replace("{v}", _apName);

  for (int i = 0; i < _APCount; i++)
  {
    page.replace("{x"+i+"}", _ssid[i]);
  }
  
  page += FPSTR(E_HTML_END);
 
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", page);
  response->addHeader(FPSTR(E_HTTP_CACHE_CONTROL), FPSTR(E_HTTP_NO_STORE));
  
#if USING_CORS_FEATURE
  // New from v1.1.0, for configure CORS Header, default to E_HTTP_CORS_ALLOW_ALL = "*"
  response->addHeader(FPSTR(E_HTTP_CORS), _CORS_Header);
#endif
  
  response->addHeader(FPSTR(E_HTTP_PRAGMA), FPSTR(E_HTTP_NO_CACHE));
  response->addHeader(FPSTR(E_HTTP_EXPIRES), "-1");
  request->send(response);

  LOGDEBUG(F("Sent wifi save page"));

  connect = true; //signal ready to connect/reset

  // Restore when Press Save WiFi
  _configPortalTimeout = DEFAULT_PORTAL_TIMEOUT;
}

// Handle shut down the server page
void ESP8266_Encompass::handleServerClose(AsyncWebServerRequest *request)
{
  LOGDEBUG(F("Server Close"));
   
  String page = FPSTR(E_HTML_HEAD_START);
  page.replace("{v}", "Close Server");
  page += FPSTR(E_HTML_SCRIPT);
  page += FPSTR(E_HTML_SCRIPT_NTP);
  page += FPSTR(E_HTML_STYLE);
  page += _customHeadElement;
  page += FPSTR(E_HTML_HEAD_END);
  page += F("<div class=\"msg\">");
  page += F("My network is <b>");
  page += WiFi_SSID();
  page += F("</b><br>");
  page += F("IP address is <b>");
  page += WiFi.localIP().toString();
  page += F("</b><br><br>");
  page += F("Portal closed...<br><br>");
  
  //page += F("Push button on device to restart configuration server!");
  
  page += FPSTR(E_HTML_END);
   
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", page);
  response->addHeader(FPSTR(E_HTTP_CACHE_CONTROL), FPSTR(E_HTTP_NO_STORE));
  
#if USING_CORS_FEATURE
  // New from v1.1.0, for configure CORS Header, default to E_HTTP_CORS_ALLOW_ALL = "*"
  response->addHeader(FPSTR(E_HTTP_CORS), _CORS_Header);
#endif
  
  response->addHeader(FPSTR(E_HTTP_PRAGMA), FPSTR(E_HTTP_NO_CACHE));
  response->addHeader(FPSTR(E_HTTP_EXPIRES), "-1");
  
  request->send(response);
  
  stopConfigPortal = true; //signal ready to shutdown config portal
  
  LOGDEBUG(F("Sent server close page"));

  // Restore when Press Save WiFi
  _configPortalTimeout = DEFAULT_PORTAL_TIMEOUT;
}

// Handle the info page
void ESP8266_Encompass::handleInfo(AsyncWebServerRequest *request)
{
  LOGDEBUG(F("Info"));

  // Disable _configPortalTimeout when someone accessing Portal to give some time to config
  _configPortalTimeout = 0;
 
  String page = FPSTR(E_HTML_HEAD_START);
  page.replace("{v}", "Info");
  page += FPSTR(E_HTML_SCRIPT);
  page += FPSTR(E_HTML_SCRIPT_NTP);
  page += FPSTR(E_HTML_STYLE);
  page += _customHeadElement;
  
  if (connect)
    page += F("<meta http-equiv=\"refresh\" content=\"5; url=/i\">");
  
  page += FPSTR(E_HTML_HEAD_END);
  
  page += F("<dl>");
  
  if (connect)
  {
    page += F("<dt>Trying to connect</dt><dd>");
    page += wifiStatus;
    page += F("</dd>");
  }

  page +=pager;
  
  page += F("<h2>WiFi Information</h2>");
  reportStatus(page);
  
  page += FPSTR(E_FLDSET_START);
  
  page += F("<h3>Device Data</h3>");
  
  page += F("<table class=\"table\">");
  page += F("<thead><tr><th>Name</th><th>Value</th></tr></thead><tbody><tr><td>Chip ID</td><td>");

  page += String(ESP.getChipId(), HEX);		//ESP.getChipId();

  page += F("</td></tr>");
  page += F("<tr><td>Flash Chip ID</td><td>");

  page += String(ESP.getFlashChipId(), HEX);		//ESP.getFlashChipId();

  page += F("</td></tr>");
  page += F("<tr><td>IDE Flash Size</td><td>");
  page += ESP.getFlashChipSize();
  page += F(" bytes</td></tr>");
  page += F("<tr><td>Real Flash Size</td><td>");

  page += ESP.getFlashChipRealSize();

  page += F(" bytes</td></tr>");
  page += F("<tr><td>Access Point IP</td><td>");
  page += WiFi.softAPIP().toString();
  page += F("</td></tr>");
  page += F("<tr><td>Access Point MAC</td><td>");
  page += WiFi.softAPmacAddress();
  page += F("</td></tr>");

  page += F("<tr><td>SSID</td><td>");
  page += WiFi_SSID();
  page += F("</td></tr>");

  page += F("<tr><td>Station IP</td><td>");
  page += WiFi.localIP().toString();
  page += F("</td></tr>");

  page += F("<tr><td>Station MAC</td><td>");
  page += WiFi.macAddress();
  page += F("</td></tr>");
  page += F("</tbody></table>");

  page += FPSTR(E_FLDSET_END);
  
#if USE_AVAILABLE_PAGES  
  page += FPSTR(E_FLDSET_START);
  
  page += FPSTR(E_HTTP_AVAILABLE_PAGES);
  
  page += FPSTR(E_FLDSET_END);
#endif

#ifdef SHOW_DEV_FOOTER
  page += F("<p/>More information about ESP8266_Encompass at");
  page += F("<p/><a href=\"https://github.com/thewhiterabbit/ESP8266_Encompass\">https://github.com/thewhiterabbit/ESP8266_Encompass</a>");
#endif
  page += FPSTR(E_HTML_END);
 
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", page);
  response->addHeader(FPSTR(E_HTTP_CACHE_CONTROL), FPSTR(E_HTTP_NO_STORE));
  
#if USING_CORS_FEATURE
  // New from v1.1.0, for configure CORS Header, default to E_HTTP_CORS_ALLOW_ALL = "*"
  response->addHeader(FPSTR(E_HTTP_CORS), _CORS_Header);
#endif
  
  response->addHeader(FPSTR(E_HTTP_PRAGMA), FPSTR(E_HTTP_NO_CACHE));
  response->addHeader(FPSTR(E_HTTP_EXPIRES), "-1");
  
  request->send(response);

  LOGDEBUG(F("Info page sent"));
}

// Handle the state page
void ESP8266_Encompass::handleState(AsyncWebServerRequest *request)
{
  LOGDEBUG(F("State - json"));
   
  String page = F("{\"Soft_AP_IP\":\"");
  page += WiFi.softAPIP().toString();
  page += F("\",\"Soft_AP_MAC\":\"");
  page += WiFi.softAPmacAddress();
  page += F("\",\"Station_IP\":\"");
  page += WiFi.localIP().toString();
  page += F("\",\"Station_MAC\":\"");
  page += WiFi.macAddress();
  page += F("\",");

  if (WiFi.psk() != "")
  {
    page += F("\"Password\":true,");
  }
  else
  {
    page += F("\"Password\":false,");
  }

  page += F("\"SSID\":\"");
  page += WiFi_SSID();
  page += F("\"}");
   
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", page);
  response->addHeader(FPSTR(E_HTTP_CACHE_CONTROL), FPSTR(E_HTTP_NO_STORE));
  
#if USING_CORS_FEATURE
  // New from v1.1.0, for configure CORS Header, default to E_HTTP_CORS_ALLOW_ALL = "*"
  response->addHeader(FPSTR(E_HTTP_CORS), _CORS_Header);
#endif
  
  response->addHeader(FPSTR(E_HTTP_PRAGMA), FPSTR(E_HTTP_NO_CACHE));
  response->addHeader(FPSTR(E_HTTP_EXPIRES), "-1");
  
  request->send(response);
  
  LOGDEBUG(F("Sent state page in json format"));
}

// Handle the reset page
void ESP8266_Encompass::handleReset(AsyncWebServerRequest *request)
{
  LOGDEBUG(F("Reset"));
    
  String page = FPSTR(E_HTML_HEAD_START);
  page.replace("{v}", "WiFi Information");
  page += FPSTR(E_HTML_SCRIPT);
  page += FPSTR(E_HTML_SCRIPT_NTP);
  page += FPSTR(E_HTML_STYLE);
  page += _customHeadElement;
  page += FPSTR(E_HTML_HEAD_END);
  page += F("Resetting");
  page += FPSTR(E_HTML_END);
    
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", page);
  response->addHeader(E_HTTP_CACHE_CONTROL, E_HTTP_NO_STORE);
  response->addHeader(E_HTTP_PRAGMA, E_HTTP_NO_CACHE);
  response->addHeader(E_HTTP_EXPIRES, "-1");
  
  request->send(response);

  LOGDEBUG(F("Sent reset page"));
  delay(5000);
  
  // Temporary fix for issue of not clearing WiFi SSID/PW from flash of ESP32
  // See https://github.com/thewhiterabbit/ESP_WiFiManager/issues/25 and https://github.com/espressif/arduino-esp32/issues/400
  resetSettings();
  //WiFi.disconnect(true); // Wipe out WiFi credentials.
  //////

  ESP.reset();
  delay(2000);
}

void ESP8266_Encompass::handleNotFound(AsyncWebServerRequest *request)
{
  if (captivePortal(request))
  {
    // If caprive portal redirect instead of displaying the error page.
    return;
  }

  String message = "File Not Found\n\n";
  
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += request->args();
  message += "\n";

  for (uint8_t i = 0; i < request->args(); i++)
  {
    message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
  }

  AsyncWebServerResponse *response = request->beginResponse( 404, "text/plain", message );
  response->addHeader(FPSTR(E_HTTP_CACHE_CONTROL), FPSTR(E_HTTP_NO_STORE)); 
  response->addHeader(FPSTR(E_HTTP_PRAGMA), FPSTR(E_HTTP_NO_CACHE));
  response->addHeader(FPSTR(E_HTTP_EXPIRES), "-1");
  
  request->send(response);
}

/**
   HTTPD redirector
   Redirect to captive portal if we got a request for another domain.
   Return true in that case so the page handler do not try to handle the request again.
*/
boolean ESP8266_Encompass::captivePortal(AsyncWebServerRequest *request)
{
  if (!isIp(request->host()))
  {
    LOGDEBUG(F("Request redirected to captive portal"));
    
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
    response->addHeader("Location", String("http://") + toStringIp(request->client()->localIP()));
    request->send(response);
       
    return true;
  }
  
  return false;
}

// start up config portal callback
void ESP8266_Encompass::setAPCallback(void(*func)(ESP8266_Encompass* myWiFiManager))
{
  _apcallback = func;
}

// start up save config callback
void ESP8266_Encompass::setSaveConfigCallback(void(*func)(void))
{
  _savecallback = func;
}

// sets a custom element to add to head, like a new style tag
void ESP8266_Encompass::setCustomHeadElement(const char* element) {
  _customHeadElement = element;
}

// if this is true, remove duplicated Access Points - defaut true
void ESP8266_Encompass::setRemoveDuplicateAPs(boolean removeDuplicates)
{
  _removeDuplicateAPs = removeDuplicates;
}

// Scan for WiFiNetworks in range and sort by signal strength
// space for indices array allocated on the heap and should be freed when no longer required
int ESP8266_Encompass::scanWifiNetworks(int **indicesptr)
{
  LOGDEBUG(F("Scanning Network"));
  
  int n = WiFi.scanNetworks();

  LOGDEBUG1(F("scanWifiNetworks: Done, Scanned Networks n ="), n); 

  //KH, Terrible bug here. WiFi.scanNetworks() returns n < 0 => malloc( negative == very big ) => crash!!!
  //In .../esp32/libraries/WiFi/src/WiFiType.h
  //#define WIFI_SCAN_RUNNING   (-1)
  //#define WIFI_SCAN_FAILED    (-2)
  //if (n == 0)
  if (n <= 0)
  {
    LOGDEBUG(F("No network found"));
    return (0);
  }
  else
  {
    // Allocate space off the heap for indices array.
    // This space should be freed when no longer required.
    int* indices = (int *)malloc(n * sizeof(int));

    if (indices == NULL)
    {
      LOGDEBUG(F("ERROR: Out of memory"));
      *indicesptr = NULL;
      return (0);
    }

    *indicesptr = indices;
   
    //sort networks
    for (int i = 0; i < n; i++)
    {
      indices[i] = i;
    }

    LOGDEBUG(F("Sorting"));

    // RSSI SORT
    // old sort
    for (int i = 0; i < n; i++)
    {
      for (int j = i + 1; j < n; j++)
      {
        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i]))
        {
          std::swap(indices[i], indices[j]);
        }
      }
    }

    LOGDEBUG(F("Removing Dup"));

    // remove duplicates ( must be RSSI sorted )
    if (_removeDuplicateAPs)
    {
      String cssid;
      for (int i = 0; i < n; i++)
      {
        if (indices[i] == -1)
          continue;

        cssid = WiFi.SSID(indices[i]);
        for (int j = i + 1; j < n; j++)
        {
          if (cssid == WiFi.SSID(indices[j]))
          {
            LOGDEBUG1("DUP AP:", WiFi.SSID(indices[j]));
            
            // set dup aps to index -1
            indices[j] = -1;
          }
        }
      }
    }

    for (int i = 0; i < n; i++)
    {
      if (indices[i] == -1)
      {
        // skip dups
        continue;
      }

      int quality = getRSSIasQuality(WiFi.RSSI(indices[i]));

      if (!(_minimumQuality == -1 || _minimumQuality < quality))
      {
        indices[i] = -1;
        LOGDEBUG(F("Skipping low quality"));
      }
    }

#if (_ENCOMPASS_LOGLEVEL_ > 2)
    for (int i = 0; i < n; i++)
    {
      if (indices[i] == -1)
      { 
        // skip dups
        continue;
      }
      else
        LOGINFO(WiFi.SSID(indices[i]));
    }
#endif

    return (n);
  }
}

int ESP8266_Encompass::getRSSIasQuality(int RSSI)
{
  int quality = 0;

  if (RSSI <= -100)
  {
    quality = 0;
  }
  else if (RSSI >= -50)
  {
    quality = 100;
  }
  else
  {
    quality = 2 * (RSSI + 100);
  }

  return quality;
}

// Is this an IP?
boolean ESP8266_Encompass::isIp(String str)
{
  for (int i = 0; i < str.length(); i++)
  {
    int c = str.charAt(i);

    if (c != '.' && (c < '0' || c > '9'))
    {
      return false;
    }
  }
  return true;
}

// IP to String
String ESP8266_Encompass::toStringIp(IPAddress ip)
{
  String res = "";
  for (int i = 0; i < 3; i++)
  {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }

  res += String(((ip >> 8 * 3)) & 0xFF);

  return res;
}

/*#ifdef ESP32 // <-- Maybe remove this & all the contained lines within
// We can't use WiFi.SSID() in ESP32 as it's only valid after connected.
// SSID and Password stored in ESP32 wifi_ap_record_t and wifi_config_t are also cleared in reboot
// Have to create a new function to store in EEPROM/SPIFFS for this purpose

String ESP8266_Encompass::getStoredWiFiSSID()
{
  if (WiFi.getMode() == WIFI_MODE_NULL)
  {
    return String();
  }

  wifi_ap_record_t info;

  if (!esp_wifi_sta_get_ap_info(&info))
  {
    return String(reinterpret_cast<char*>(info.ssid));
  }
  else
  {
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    return String(reinterpret_cast<char*>(conf.sta.ssid));
  }

  return String();
}

String ESP8266_Encompass::getStoredWiFiPass()
{
  if (WiFi.getMode() == WIFI_MODE_NULL)
  {
    return String();
  }

  wifi_config_t conf;
  esp_wifi_get_config(WIFI_IF_STA, &conf);
  
  return String(reinterpret_cast<char*>(conf.sta.password));
}
#endif
*/


