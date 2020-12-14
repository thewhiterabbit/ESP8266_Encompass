//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Encompass
{
  public:

    Encompass(AsyncWebServer * webserver, DNSServer *dnsserver, const char *iHostname = "");

    ~Encompass();
    
    void          scan();
    String        scanModal();
    void          loop();
    void          safeLoop();
    void          criticalLoop();
    String        infoAsString();

    // Can use with STA staticIP now
    boolean       autoConnect();
    boolean       autoConnect(char const *apName, char const *apPassword = NULL);
    //////

    // If you want to start the config portal
    boolean       startConfigPortal();
    boolean       startConfigPortal(char const *apName, char const *apPassword = NULL);
    void          startConfigPortalModeless(char const *apName, char const *apPassword);


    // get the AP name of the config portal, so it can be used in the callback
    String        getConfigPortalSSID();
    // get the AP password of the config portal, so it can be used in the callback
    String        getConfigPortalPW();

    void          resetSettings();

    //sets timeout before webserver loop ends and exits even if there has been no setup.
    //usefully for devices that failed to connect at some point and got stuck in a webserver loop
    //in seconds setConfigPortalTimeout is a new name for setTimeout
    void          setConfigPortalTimeout(unsigned long seconds);
    void          setTimeout(unsigned long seconds);

    //sets timeout for which to attempt connecting, usefull if you get a lot of failed connects
    void          setConnectTimeout(unsigned long seconds);


    void          setDebugOutput(boolean debug);
    //defaults to not showing anything under 8% signal quality if called
    void          setMinimumSignalQuality(int quality = 8);
    
    // To enable dynamic/random channel
    int           setConfigPortalChannel(int channel = 1);
    //////
    
    //sets a custom ip /gateway /subnet configuration
    void          setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn);
    //sets config for a static IP
    void          setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn);

    #if USE_CONFIGURABLE_DNS
    void          setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn,
                                            IPAddress dns_address_1, IPAddress dns_address_2);
    #endif

    //called when AP mode and config portal is started
    void          setAPCallback(void(*func)(Encompass*));
    //called when settings have been changed and connection was successful
    void          setSaveConfigCallback(void(*func)(void));

    //adds a custom parameter
    bool 				  addDataField(E_DataFields *p);

    //if this is set, it will exit after config, even if connection is unsucessful.
    void          setBreakAfterConfig(boolean shouldBreak);
    
    //if this is set, try WPS setup when starting (this will delay config portal for up to 2 mins)
    //TODO
    //if this is set, customise style
    void          setCustomHeadElement(const char* element);
    
    //if this is true, remove duplicated Access Points - defaut true
    void          setRemoveDuplicateAPs(boolean removeDuplicates);
    
    //Scan for WiFiNetworks in range and sort by signal strength
    //space for indices array allocated on the heap and should be freed when no longer required
    int           scanWifiNetworks(int **indicesptr);

    // return SSID of router in STA mode got from config portal. NULL if no user's input //KH
    String				getSSID(void)
    {
      return _ssid[0];
    }

    // return password of router in STA mode got from config portal. NULL if no user's input //KH
    String				getPW(void)
    {
      return _pass[0];
    }
    
    #define MAX_WIFI_CREDENTIALS        2
    
    String				getSSID(uint8_t i)
    {
      if (i == 0 || i == 1)
        return _ssid[i];
      else     
        return String("");
    }
    
    String				getPW(uint8_t i)
    {
      if (i == 0 || i == 1)
        return _pass[i];
      else     
        return String("");
    }
    //////
    
    // New from v1.1.1, for configure CORS Header, default to E_HTTP_CORS_ALLOW_ALL = "*"
    #if USING_CORS_FEATURE
    void setCORSHeader(const char* CORSHeaders)
    {     
      _CORS_Header = CORSHeaders;

      LOGWARN1(F("Set CORS Header to : "), _CORS_Header);
    }
    
    const char* getCORSHeader(void)
    {
      return _CORS_Header;
    }
    #endif     

    //returns the list of DataFields
    E_DataFields** getDataFields();
    
    // returns the DataFields Count
    int           getDataFieldsCount();

    const char*   getStatus(int status);

    String WiFi_SSID(void)
    {
      return WiFi.SSID();
    }

    String WiFi_Pass(void)
    {
      return WiFi.psk();
    }

    void setHostname(void)
    {
      if (RFC952_hostname[0] != 0)
      {
        WiFi.hostname(RFC952_hostname);
      }
    }

  private:
  
    DNSServer      *dnsServer;

    AsyncWebServer *server;

    boolean         _modeless;
    int             scannow;
    int             shouldscan;
    boolean         needInfo = true;
    String          pager;
    wl_status_t     wifiStatus;

    #define RFC952_HOSTNAME_MAXLEN      24
    char RFC952_hostname[RFC952_HOSTNAME_MAXLEN + 1];

    char* getRFC952_hostname(const char* iHostname);

    void          setupConfigPortal();
    void          startWPS();

    const char*   _apName               = "no-net";
    const char*   _apPassword           = NULL;
    
    String        _ssid[];
    String        _pass[];

    // Timezone info
    String        _timezoneName         = "";

    unsigned long _configPortalTimeout  = 0;

    unsigned long _connectTimeout       = 0;
    unsigned long _configPortalStart    = 0;

    int                 numberOfNetworks;
    int                 *networkIndices;
    
    WiFiResult          *wifiSSIDs;
    wifi_ssid_count_t   wifiSSIDCount;
    boolean             wifiSSIDscan;
    
    // To enable dynamic/random channel
    // default to channel 1
    #define MIN_WIFI_CHANNEL      1
    #define MAX_WIFI_CHANNEL      11

    int _WiFiAPChannel = 1;
    //////

    IPAddress     _ap_static_ip;
    IPAddress     _ap_static_gw;
    IPAddress     _ap_static_sn;
    IPAddress     _sta_static_ip = IPAddress(0, 0, 0, 0);
    IPAddress     _sta_static_gw;
    IPAddress     _sta_static_sn;

    #if USE_CONFIGURABLE_DNS
    IPAddress     _sta_static_dns1;
    IPAddress     _sta_static_dns2;
    #endif

    int           _DataFieldCount              = 0;
    int           _minimumQuality           = -1;
    boolean       _removeDuplicateAPs       = true;
    boolean       _shouldBreakAfterConfig   = false;
    boolean       _tryWPS                   = false;

    const char*   _customHeadElement        = "";

    int           status                    = WL_IDLE_STATUS;
    
    // New from v1.1.0, for configure CORS Header, default to E_HTTP_CORS_ALLOW_ALL = "*"
    #if USING_CORS_FEATURE
    const char*     _CORS_Header            = E_HTTP_CORS_ALLOW_ALL;   //"*";
    #endif   
    //////

    void          setWifiStaticIP(void);
    
    // New v1.1.0
    int           reconnectWifi(void);
    //////
    
    int           connectWifi(String ssid = "", String pass = "");
    
    wl_status_t   waitForConnectResult();
    
    void          setInfo();
    String        networkListAsString();
    
    void          handleRoot(AsyncWebServerRequest *request);
    void          handleWifi(AsyncWebServerRequest *request);
    void          handleSave(AsyncWebServerRequest *request);
    void          handleServerClose(AsyncWebServerRequest *request);
    void          handleInfo(AsyncWebServerRequest *request);
    void          handleState(AsyncWebServerRequest *request);
    void          handleReset(AsyncWebServerRequest *request);
    void          handleNotFound(AsyncWebServerRequest *request);
    boolean       captivePortal(AsyncWebServerRequest *request);   
    
    void          reportStatus(String &page);

    // DNS server
    const byte    DNS_PORT = 53;

    //helpers
    int           getRSSIasQuality(int RSSI);
    boolean       isIp(String str);
    String        toStringIp(IPAddress ip);

    boolean       connect;
    boolean       stopConfigPortal = false;
    
    boolean       _debug = false;     //true;
    
    void(*_apcallback)(Encompass*) = NULL;
    void(*_savecallback)(void)                = NULL;

    int                     _MAX_DATA_FIELDS;
    E_DataFields  _DataField;

    template <typename Generic>
    void          DEBUG_WM(Generic text);

    template <class T>
    auto optionalIPFromString(T *obj, const char *s) -> decltype(obj->fromString(s)) 
    {
      return  obj->fromString(s);
    }
    
    auto optionalIPFromString(...) -> bool 
    {
      LOGINFO("NO fromString METHOD ON IPAddress, you need ESP8266 core 2.1.0+ for Custom IP configuration to work.");
      return false;
    }
};