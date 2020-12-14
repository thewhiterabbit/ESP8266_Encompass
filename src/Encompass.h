/*
  Encompass.h
  For ESP8266 boards

  Encompass is a Twin library for the ESP8266 & ESP32/Arduino platform that enables IOT/WiFi controlled component creators to
  build intuitive public facing UIs that manage WiFi Credentials, DNS Settings, Device Settings, and display device information.

  What this library does:
    1. Manages WiFi Credentials for the device's host connection.
    2. Manages Access Point Credentials for the device.
    3. Provides optional management for Custom DNS Settings for the device.
    4. Provides a device based Web Server.
    5. Provides a blank canvas for creating stunning custom Web UIs/UXs for devices using Bootstrap, Foundation, Pure, etc..

  Modified from 
  1. Tzapu                https://github.com/tzapu/WiFiManager
  2. Ken Taylor           https://github.com/kentaylor
  3. Alan Steremberg      https://github.com/alanswx/ESPAsyncWiFiManager
  4. Khoi Hoang           https://github.com/khoih-prog/ESPAsync_WiFiManager

  Built by A. K. N.       https://github.com/thewhiterabbit/Encompass

  License to be determined in the future.
  
  Version: 1.0.1

  Version   Modified By             Date          Comments
  -------   ---------------------   ----------    -----------
  1.0.0     A. K. N.                12/07/2020    Initial edits after fork from Khoi Hoang - Removed all ESP32 code/includes.
                                                  These twin libraries will be to each their own compatability.

  1.0.1     A. K. N.                12/13/2020    Massive overhaul - Moved all classes to includes/class for best practice & organization,
                                                  removed bloat, streamlined HTML Blocks for dynamic use, miscellaneous restructuring
*/

#pragma     once
#define     ENCOMPASS_VERSION     "v1.0.1"
#include    "Debug.h"
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HTML Page Static Constants
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char HTTP_200[]             PROGMEM   = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
const char HTML_DOCTYPE[]         PROGMEM   = "<!DOCTYPE html>";
const char HTML_HEAD_OPEN[]       PROGMEM   = "<head>";
const char HTML_META_VIEWPORT[]   PROGMEM   = "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
const char HTML_HEAD_CLOSE[]      PROGMEM   = "</head>";
const char HTML_BODY_OPEN[]       PROGMEM   = "<body>";
const char HTML_CONT_DIV_OPEN[]   PROGMEM   = "<div class=\"container\">";
const char HTML_PAGE_DIV_OPEN[]   PROGMEM   = "<div class=\"page\">";
const char HTML_DIV_CLOSE[]       PROGMEM   = "</div>";
const char HTML_BODY_CLOSE[]      PROGMEM   = "</body>";
const char HTML_CLOSE[]           PROGMEM   = "</html>";
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HTML Page Dynamic Constants
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HTML Open Tag - {l} = language
const char HTML_OPEN[]            PROGMEM   = "<html lang=\"{l}\">";
// Document Title - {t} = title text
const char HTML_TITLE[]           PROGMEM   = "<title>{t}</title>";
// Style Block - {s} = style definition/s;
const char HTML_STYLE_BLOCK[]     PROGMEM   = "<style>{s}</style>";
// Style Definition - {t} = target/s, {p} = parameter/s: value;
const char HTML_STYLE_DEF[]       PROGMEM   = "{t}{{p}}";
// Script Block - {l} = src location, or, {s} = minified script
const char HTML_SCRIPT_BLOCK[]    PROGMEM   = "<script{l}>{s}</script>";
// Div Block - {c} = class, {dc} = div content
const char HTML_DIV_BLOCK[]       PROGMEM   = "<div{c}>{dc}</div>";
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HTML Blocks
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fieldset Element Open
const char FLDSET_START[]         PROGMEM = "<fieldset>";
// Fieldset Element Close
const char FLDSET_END[]           PROGMEM = "</fieldset>";
// Menu Item Button - {a} = action (page URI), {m} = method, {t} = button text
const char HTML_UI_MENU_ITEM[]    PROGMEM = "<form action=\"{a}\" method=\"{m}\"><button class=\"btn\">{t}</button></form><br>";
// Form Opening Element - {m} = method, {a} = action
const char HTML_FORM_START[]      PROGMEM = "<form method=\"{m}\" action=\"{a}\">";
// Form Closing Element
const char HTML_FORM_END[]        PROGMEM = "</form>";
// Label Element - {i} = for field id, {t} = text, {s} = style
const char HTML_FORM_LABEL[]      PROGMEM = "<label for=\"{i}\"{s}>{t}</label>";
// Form Element Open - {e} = element, {n} = name, {i} = id, {l} = length
const char HTML_ELEMENT_START[]   PROGMEM = "<{e} name=\"{n}\" id=\"{i}\" placeholder=\"{p}\" value=\"{v}\"{s}>";
const char HTML_ELEMENT_END[]     PROGMEM = "</{e}>";
// Option Element - {v} = value, {s} = selected, {d} = disabled, {t} = text
const char HTML_OPTION[]          PROGMEM = "<option value=\"{v}\"{s}{d}>{t}</option>";
const char HTML_INPUT[]           PROGMEM = "<input type=\"{t}\" id=\"{i}\" name=\"{n}\" value=\"{v}\"{c}{s}>";
const char HTML_BUTTON[]          PROGMEM = "<button class=\"btn\" type=\"{t}\">Save</button>";
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Response Messages
const char HTML_SAVED[]           PROGMEM = "<div class=\"msg\"><h3>Wifi Credentials Saved</h3><p>Connecting {d} to the {n} network.</p></div>";
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// WiFi List Item - Template for building the list of detected networks
const char WIFI_LIST_ITEM[]       PROGMEM = "<div><a href=\"#p\" onclick=\"c(this)\">{v}</a>&nbsp;<span class=\"q {i}\">{r}%</span></div>";
const char JSON_SSID_ITEM[]       PROGMEM = "{\"SSID\":\"{v}\", \"Encryption\":{i}, \"Quality\":\"{r}\"}";

const char HTTP_HEAD_CL[]         PROGMEM = "Content-Length";
const char HTTP_HEAD_CT[]         PROGMEM = "text/html";
const char HTTP_HEAD_CT2[]        PROGMEM = "text/plain";
const char HTTP_CACHE_CONTROL[]   PROGMEM = "Cache-Control";
const char HTTP_NO_STORE[]        PROGMEM = "no-cache, no-store, must-revalidate";
const char HTTP_PRAGMA[]          PROGMEM = "Pragma";
const char HTTP_NO_CACHE[]        PROGMEM = "no-cache";
const char HTTP_EXPIRES[]         PROGMEM = "Expires";
const char HTTP_CORS[]            PROGMEM = "Access-Control-Allow-Origin";
const char HTTP_CORS_ALLOW_ALL[]  PROGMEM = "*";
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "include/class/DataField.cls"
#include "include/class/WiFiResult.cls"
#include "include/class/Encompass.cls"
#include "include/class/Impl.h"