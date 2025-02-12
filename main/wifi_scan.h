#ifndef _WIFI_SCAN_H
#define _WIFI_SCAN_H

#include "platform.h"


#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 64

void wifi_scan_m(void);
// void wifi_init(void);
void wifi_connect(const char *ssid, const char *password);
#endif