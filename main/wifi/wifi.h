#include <stdbool.h>
#define EXAMPLE_MAX_STA_CONN       5

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_MAXIMUM_RETRY      10
#include "esp_err.h"

/**
 * Setup wifi 
 * 
 * Connect to provided network or open an access point for configuration
 */
esp_err_t wifi_init_softap();

