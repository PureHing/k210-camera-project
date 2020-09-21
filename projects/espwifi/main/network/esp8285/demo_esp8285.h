#ifndef __DEMO_ESP8285_h
#define __DEMO_ESP8285_h

#include "global_config.h"

/* clang-format off */
#if CONFIG_NET_ENABLE
#if CONFIG_NET_ESP8285
#define CONFIG_KEY_SHORT_QRCODE             (1)
#define CONFIG_NET_DEMO_MQTT                (1)
#define CONFIG_NET_DEMO_HTTP_GET            (1)
#define CONFIG_NET_DEMO_HTTP_POST           (1)
#endif
#endif
/* clang-format on */

#endif
