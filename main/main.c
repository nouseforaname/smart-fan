// ota includes

#include "esp_spi_flash.h"
#include "esp_log.h"
#include "stdint.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "stdio.h"
#include "driver/gpio.h"


// softap includes
#include <secrets.h>
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "esp_system.h"
//#include "esp_wifi.h"
//#include "esp_event.h"
//#include "esp_log.h"
//#include "nvs_flash.h"

#include "ota.h"
#include "wifi.h"
#include "server.h"
#include "persistence/persistence.h"

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

// include config for system
#include "secrets.h"

esp_ota_firm_t ota_firm;

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");



void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_softap();
    ESP_LOGI(TAG, "GPIO config updated."); 
    init_persistence();
    ESP_ERROR_CHECK(init_http());


    // Initialize OTA-update
    ESP_ERROR_CHECK(init_ota(&ota_firm));
    ESP_LOGI(TAG, "Succesfully inialized");
}
