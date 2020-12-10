/*
  ESP8266_Encompass.h
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

#pragma     once
#define     ESP8266_ENCOMPASS_VERSION     "v1.0.0"
#include    "ESP8266_Encompass_Debug.h"
#include    <ESP8266WiFi.h>
#include    <ESPAsyncWebServer.h>
#include    <DNSServer.h>
#include    <memory>
#undef      min
#undef      max
#include    <algorithm>

typedef int wifi_ssid_count_t;

extern "C"
{
  #include "user_interface.h"
}

#define ESP_getChipId()   (ESP.getChipId())

#define E_LABEL_BEFORE 1
#define E_LABEL_AFTER 2
#define E_NO_LABEL 0

/** Handle CORS in pages */
// Default false for using only whenever necessary to avoid security issue when using CORS (Cross-Origin Resource Sharing)
#ifndef USING_CORS_FEATURE
  // Contributed by AlesSt (https://github.com/AlesSt) to solve AJAX CORS protection problem of API redirects on client side
  // See more in https://github.com/khoih-prog/ESP8266_ENCOMPASS/issues/27 and https://en.wikipedia.org/wiki/Cross-origin_resource_sharing
  #define USING_CORS_FEATURE     false
#endif

#ifndef TIME_BETWEEN_MODAL_SCANS
  // Default to 30s
  #define TIME_BETWEEN_MODAL_SCANS          30000
#endif

#ifndef TIME_BETWEEN_MODELESS_SCANS
  // Default to 60s
  #define TIME_BETWEEN_MODELESS_SCANS       60000
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HTML Page Head Constants
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char E_HTTP_200[]             PROGMEM   = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
const char E_HTML_HEAD_START[]      PROGMEM   = "<!DOCTYPE html>"+
                                                "<html lang=\"en\">"+
                                                "<head>"+
                                                "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>"+
                                                "<title>{v}</title>";
const char E_HTML_STYLE_START[]     PROGMEM   = "<style>";
const char E_HTML_STYLE_CONTENT[]   PROGMEM   = "div{padding:2px;font-size:1em;}"+
                                                "body,textarea,input,select{background: 0;border-radius: 0;font: 16px sans-serif;margin: 0}"+
                                                "textarea,input,select{outline: 0;font-size: 14px;border: 1px solid #ccc;padding: 8px;width: 90%}"+
                                                ".btn a{text-decoration: none}"+
                                                ".container{margin: auto;width: 90%}"+
                                                "@media(min-width:1200px){.container{margin: auto;width: 30%}}"+
                                                "@media(min-width:768px) and (max-width:1200px){.container{margin: auto;width: 50%}}"+
                                                ".btn,h2{font-size: 2em}"+
                                                "h1{font-size: 3em}"+
                                                ".btn{background: #0ae;border-radius: 4px;border: 0;color: #fff;cursor: pointer;display: inline-block;margin: 2px 0;padding: 10px 14px 11px;width: 100%}"+
                                                ".btn:hover{background: #09d}"+
                                                ".btn:active,.btn:focus{background: #08b}"+
                                                "label>*{display: inline}"+
                                                "form>*{display: block;margin-bottom: 10px}"+
                                                "textarea:focus,input:focus,select:focus{border-color: #5ab}"+
                                                ".msg{background: #def;border-left: 5px solid #59d;padding: 1.5em}"+
                                                ".q{float: right;width: 64px;text-align: right}"+
                                                ".l{background: url('data:image/png;base64,"+
                                                "iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho"+
                                                "6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19h"+
                                                "mdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg=="+
                                                "') no-repeat left center;background-size: 1em}"+
                                                "input[type='checkbox']{float: left;width: 20px}"+
                                                ".table td{padding:.5em;text-align:left}"+
                                                ".table tbody>:nth-child(2n-1){background:#ddd}"+
                                                "fieldset{border-radius:0.5rem;margin:0px;}";
const char E_HTML_STYLE_END[]       PROGMEM = "</style>";
const char E_HTML_SCRIPT_START[]    PROGMEM = "<script>";
const char E_HTML_SCRIPT_CONTENT[]  PROGMEM = "function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();document.getElementById('s1').value=l.innerText||l.textContent;document.getElementById('p1').focus();}";
const char E_HTML_SCRIPT_END[]      PROGMEM = "</script>";
const char E_HTML_HEAD_END[]        PROGMEM = "</head><body><div class=\"container\"><div style=\"text-align:left;display:inline-block;min-width:260px;\">";
const char E_HTML_END[]             PROGMEM = "</div></body></html>";
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// To permit disable or configure NTP from sketch
#ifndef USE_ESP8266_ENCOMPASS_NTP
  // To enable NTP config
  #define USE_ESP8266_ENCOMPASS_NTP     true
#endif

#if USE_ESP8266_ENCOMPASS_NTP

  const char E_HTTP_SCRIPT_NTP_MSG[] PROGMEM = "<p>Your timezone is : <b><label id='timezone'></b><script>document.getElementById('timezone').innerHTML = timezone.name();</script></p>";
  // Cloudflare NTP Script
  const char E_HTML_SCRIPT_NTP[] PROGMEM = "<script src='https://cdnjs.cloudflare.com/ajax/libs/jstimezonedetect/1.0.4/jstz.min.js'></script><script>var timezone=jstz.determine();console.log('Your timezone is:' + timezone.name());document.getElementById('timezone').innerHTML = timezone.name();</script>";

#else
  const char E_HTML_SCRIPT_NTP_MSG[] PROGMEM   = "";
  const char E_HTML_SCRIPT_NTP[] PROGMEM       = "";
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HTML Blocks
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fieldset Element Open
const char E_FLDSET_START[]         PROGMEM = "<fieldset>";
// Fieldset Element Close
const char E_FLDSET_END[]           PROGMEM = "</fieldset>";
// Menu Item Button - {a} = action (page URI), {m} = method, {t} = button text
const char E_HTML_UI_MENU_ITEM[]    PROGMEM = "<form action=\"{a}\" method=\"{m}\"><button class=\"btn\">{t}</button></form><br>";
// Form Opening Element - {m} = method, {a} = action
const char E_HTML_FORM_START[]      PROGMEM = "<form method=\"{m}\" action=\"{a}\">";
// Form Closing Element
const char E_HTML_FORM_END[]        PROGMEM = "</form>";
// Label Element - {i} = for field id, {t} = text, {s} = style
const char E_HTML_FORM_LABEL[]      PROGMEM = "<label for=\"{i}\"{s}>{t}</label>";
// Form Element Open - {e} = element, {n} = name, {i} = id, {l} = length
const char E_HTML_ELEMENT_START[]   PROGMEM = "<{e} name=\"{n}\" id=\"{i}\" placeholder=\"{p}\" value=\"{v}\"{s}>";
const char E_HTML_ELEMENT_END[]     PROGMEM = "</{e}>";
// Option Element - {v} = value, {s} = selected, {d} = disabled, {t} = text
const char E_HTML_OPTION[]          PROGMEM = "<option value=\"{v}\"{s}{d}>{t}</option>";
const char E_HTML_INPUT[]           PROGMEM = "<input type=\"{t}\" id=\"{i}\" name=\"{n}\" value=\"{v}\"{c}{s}>";
const char E_HTML_BUTTON[]          PROGMEM = "<button class=\"btn\" type=\"{t}\">Save</button>";
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Response Messages
// TODO: Create customization ability
const char E_HTML_SAVED[]           PROGMEM = "<div class=\"msg\"><h3>Wifi Credentials Saved</h3><p>Connecting {d} to the {n} network.</p></div>";
//////


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// WiFi List Item - Template for building the list of detected networks
const char E_WIFI_LIST_ITEM[]       PROGMEM = "<div><a href=\"#p\" onclick=\"c(this)\">{v}</a>&nbsp;<span class=\"q {i}\">{r}%</span></div>";

const char JSON_SSID_ITEM[]         PROGMEM = "{\"SSID\":\"{v}\", \"Encryption\":{i}, \"Quality\":\"{r}\"}";

const char E_HTTP_HEAD_CL[]         PROGMEM = "Content-Length";
const char E_HTTP_HEAD_CT[]         PROGMEM = "text/html";
const char E_HTTP_HEAD_CT2[]        PROGMEM = "text/plain";

//KH Add repeatedly used const
const char E_HTTP_CACHE_CONTROL[]   PROGMEM = "Cache-Control";
const char E_HTTP_NO_STORE[]        PROGMEM = "no-cache, no-store, must-revalidate";
const char E_HTTP_PRAGMA[]          PROGMEM = "Pragma";
const char E_HTTP_NO_CACHE[]        PROGMEM = "no-cache";
const char E_HTTP_EXPIRES[]         PROGMEM = "Expires";
const char E_HTTP_CORS[]            PROGMEM = "Access-Control-Allow-Origin";
const char E_HTTP_CORS_ALLOW_ALL[]  PROGMEM = "*";

#if USE_AVAILABLE_PAGES
const char E_HTTP_AVAILABLE_PAGES[] PROGMEM = "<h3>Available Pages</h3><table class=\"table\"><thead><tr><th>Page</th><th>Function</th></tr></thead><tbody><tr><td><a href=\"/\">/</a></td><td>Menu page.</td></tr><tr><td><a href=\"/wifi\">/wifi</a></td><td>Show WiFi scan results and enter WiFi configuration.</td></tr><tr><td><a href=\"/save\">/save</a></td><td>Save WiFi configuration information and configure device. Needs variables supplied.</td></tr><tr><td><a href=\"/close\">/close</a></td><td>Close the configuration server and configuration WiFi network.</td></tr><tr><td><a href=\"/i\">/i</a></td><td>This page.</td></tr><tr><td><a href=\"/r\">/r</a></td><td>Delete WiFi configuration and reboot. ESP device will not reconnect to a network until new WiFi configuration data is entered.</td></tr><tr><td><a href=\"/state\">/state</a></td><td>Current device state in JSON format. Interface for programmatic WiFi configuration.</td></tr></table>";
#else
const char E_HTTP_AVAILABLE_PAGES[] PROGMEM = "";
#endif

#define ENCOMPASS_MAX_DATA_FIELDS 20

// Thanks to @Amorphous for the feature and code
// (https://community.blynk.cc/t/esp-wifimanager-for-esp32-and-esp8266/42257/13)
// To enable to configure from sketch
#ifndef USE_CONFIGURABLE_DNS
  #define USE_CONFIGURABLE_DNS      true
#endif

// To permit autoConnect() to use STA static IP or DHCP IP.
#ifndef AUTOCONNECT_NO_INVALIDATE
  #define AUTOCONNECT_NO_INVALIDATE true
#endif

/////////////////////////////////////////////////////////////////////////////

class Encompass_DataFields
{
  public:
  
    Encompass_DataFields(const char *custom);
    Encompass_DataFields(const char *id, const char *placeholder, const char *defaultValue, int length);
    Encompass_DataFields(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom);
    Encompass_DataFields(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom, int labelPlacement);

    ~Encompass_DataFields();

    const char *getID();
    const char *getValue();
    const char *getPlaceholder();
    int         getValueLength();
    int         getLabelPlacement();
    const char *getCustomHTML();
    
  private:
  
    const char *_id;
    const char *_placeholder;
    char       *_value;
    int         _length;
    int         _labelPlacement;
    const char *_customHTML;

    void init(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom, int labelPlacement);

    friend class ESP8266_Encompass;
};

#define DEFAULT_PORTAL_TIMEOUT  	60000L

// To permit disable/enable StaticIP configuration in Config Portal from sketch. Valid only if DHCP is used.
// You have to explicitly specify false to disable the feature.
#ifndef USE_STATIC_IP_CONFIG_IN_CP
  #define USE_STATIC_IP_CONFIG_IN_CP          true
#endif

/////////////////////////////////////////////////////////////////////////////

class WiFiResult
{
  public:
    bool duplicate;
    String SSID;
    uint8_t encryptionType;
    int32_t RSSI;
    uint8_t* BSSID;
    int32_t channel;
    bool isHidden;

    WiFiResult()
    {
    }
};

/////////////////////////////////////////////////////////////////////////////

class ESP8266_Encompass
{
  public:

    ESP8266_Encompass(AsyncWebServer * webserver, DNSServer *dnsserver, const char *iHostname = "");

    ~ESP8266_Encompass();
    
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
    void startConfigPortalModeless(char const *apName, char const *apPassword);


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
    void          setAPCallback(void(*func)(ESP8266_Encompass*));
    //called when settings have been changed and connection was successful
    void          setSaveConfigCallback(void(*func)(void));

    //adds a custom parameter
    bool 				  addDataField(Encompass_DataFields *p);

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
    Encompass_DataFields** getDataFields();
    
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
    #define MAX_WIFI_CHANNEL      11    // Channel 12,13 is flaky, because of bad number 13 ;-)

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
    
    void(*_apcallback)(ESP8266_Encompass*) = NULL;
    void(*_savecallback)(void)                = NULL;

    int                     _MAX_DATA_FIELDS;
    Encompass_DataFields**  _DataField;

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


#include "ESP8266_Encompass-Impl.h"