/*
  Debug.h
  For ESP8266 boards

  Encompass is a Twin library for the ESP8266 & ESP32/Arduino platform that enables IOT/WiFi controlled component creators to
  build intuitive public facing UIs that manage WiFi Credentials, DNS Settings, Device Settings, and display device information.

  Modified from 
  1. Tzapu                https://github.com/tzapu/WiFiManager
  2. Ken Taylor           https://github.com/kentaylor
  3. Alan Steremberg      https://github.com/alanswx/ESPAsyncWiFiManager
  4. Khoi Hoang           https://github.com/khoih-prog/ESPAsync_WiFiManager

  Original by Khoi Hoang  https://github.com/khoih-prog

  Modified by A. K. N.    https://github.com/thewhiterabbit/Encompass

  License to be determined in the future.
  
  Version: 1.0.1

  Version   Modified By             Date          Comments
  -------   ---------------------   ----------    -----------
  1.0.0     A. K. N.                12/07/2020    Initial edits after fork from Khoi Hoang - Removed all ESP32 code/includes,
                                                  removed examples.
                                                  These twin libraries will be to each their own compatability.

  1.0.1     A. K. N.                12/13/2020    Renamed (shortened) contsants
*/

#pragma once

#ifdef ENCOMPASS_DEBUG_PORT
  #define DBG_PORT      ENCOMPASS_DEBUG_PORT
#else
  #define DBG_PORT      Serial
#endif

// Change _ENCOMPASS_LOGLEVEL_ to set tracing and logging verbosity
// 0: DISABLED: no logging
// 1: ERROR: errors
// 2: WARN: errors and warnings
// 3: INFO: errors, warnings and informational (default)
// 4: DEBUG: errors, warnings, informational and debug

#ifndef _ENCOMPASS_LOGLEVEL_
  #define _ENCOMPASS_LOGLEVEL_       0
#endif

#define LOGERROR(x)         if(_ENCOMPASS_LOGLEVEL_>0) { DBG_PORT.print("[WM] "); DBG_PORT.println(x); }
#define LOGERROR0(x)        if(_ENCOMPASS_LOGLEVEL_>0) { DBG_PORT.print(x); }
#define LOGERROR1(x,y)      if(_ENCOMPASS_LOGLEVEL_>0) { DBG_PORT.print("[WM] "); DBG_PORT.print(x); DBG_PORT.print(" "); DBG_PORT.println(y); }
#define LOGERROR2(x,y,z)    if(_ENCOMPASS_LOGLEVEL_>0) { DBG_PORT.print("[WM] "); DBG_PORT.print(x); DBG_PORT.print(" "); DBG_PORT.print(y); DBG_PORT.print(" "); DBG_PORT.println(z); }
#define LOGERROR3(x,y,z,w)  if(_ENCOMPASS_LOGLEVEL_>0) { DBG_PORT.print("[WM] "); DBG_PORT.print(x); DBG_PORT.print(" "); DBG_PORT.print(y); DBG_PORT.print(" "); DBG_PORT.print(z); DBG_PORT.print(" "); DBG_PORT.println(w); }

#define LOGWARN(x)          if(_ENCOMPASS_LOGLEVEL_>1) { DBG_PORT.print("[WM] "); DBG_PORT.println(x); }
#define LOGWARN0(x)         if(_ENCOMPASS_LOGLEVEL_>1) { DBG_PORT.print(x); }
#define LOGWARN1(x,y)       if(_ENCOMPASS_LOGLEVEL_>1) { DBG_PORT.print("[WM] "); DBG_PORT.print(x); DBG_PORT.print(" "); DBG_PORT.println(y); }
#define LOGWARN2(x,y,z)     if(_ENCOMPASS_LOGLEVEL_>1) { DBG_PORT.print("[WM] "); DBG_PORT.print(x); DBG_PORT.print(" "); DBG_PORT.print(y); DBG_PORT.print(" "); DBG_PORT.println(z); }
#define LOGWARN3(x,y,z,w)   if(_ENCOMPASS_LOGLEVEL_>1) { DBG_PORT.print("[WM] "); DBG_PORT.print(x); DBG_PORT.print(" "); DBG_PORT.print(y); DBG_PORT.print(" "); DBG_PORT.print(z); DBG_PORT.print(" "); DBG_PORT.println(w); }

#define LOGINFO(x)          if(_ENCOMPASS_LOGLEVEL_>2) { DBG_PORT.print("[WM] "); DBG_PORT.println(x); }
#define LOGINFO0(x)         if(_ENCOMPASS_LOGLEVEL_>2) { DBG_PORT.print(x); }
#define LOGINFO1(x,y)       if(_ENCOMPASS_LOGLEVEL_>2) { DBG_PORT.print("[WM] "); DBG_PORT.print(x); DBG_PORT.print(" "); DBG_PORT.println(y); }
#define LOGINFO2(x,y,z)     if(_ENCOMPASS_LOGLEVEL_>2) { DBG_PORT.print("[WM] "); DBG_PORT.print(x); DBG_PORT.print(" "); DBG_PORT.print(y); DBG_PORT.print(" "); DBG_PORT.println(z); }
#define LOGINFO3(x,y,z,w)   if(_ENCOMPASS_LOGLEVEL_>2) { DBG_PORT.print("[WM] "); DBG_PORT.print(x); DBG_PORT.print(" "); DBG_PORT.print(y); DBG_PORT.print(" "); DBG_PORT.print(z); DBG_PORT.print(" "); DBG_PORT.println(w); }

#define LOGDEBUG(x)         if(_ENCOMPASS_LOGLEVEL_>3) { DBG_PORT.print("[WM] "); DBG_PORT.println(x); }
#define LOGDEBUG0(x)        if(_ENCOMPASS_LOGLEVEL_>3) { DBG_PORT.print(x); }
#define LOGDEBUG1(x,y)      if(_ENCOMPASS_LOGLEVEL_>3) { DBG_PORT.print("[WM] "); DBG_PORT.print(x); DBG_PORT.print(" "); DBG_PORT.println(y); }
#define LOGDEBUG2(x,y,z)    if(_ENCOMPASS_LOGLEVEL_>3) { DBG_PORT.print("[WM] "); DBG_PORT.print(x); DBG_PORT.print(" "); DBG_PORT.print(y); DBG_PORT.print(" "); DBG_PORT.println(z); }
#define LOGDEBUG3(x,y,z,w)  if(_ENCOMPASS_LOGLEVEL_>3) { DBG_PORT.print("[WM] "); DBG_PORT.print(x); DBG_PORT.print(" "); DBG_PORT.print(y); DBG_PORT.print(" "); DBG_PORT.print(z); DBG_PORT.print(" "); DBG_PORT.println(w); }

