#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_err.h"

#define TAG_SPIFF "event_spiff"

esp_err_t init_persistence(){

    ESP_LOGI(TAG_SPIFF, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
  esp_err_t ret = esp_vfs_spiffs_register(&conf);
  if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
          ESP_LOGE(TAG_SPIFF, "Failed to mount or format filesystem");
      } else if (ret == ESP_ERR_NOT_FOUND) {
          ESP_LOGE(TAG_SPIFF, "Failed to find SPIFFS partition");
          unsigned long total = 0, used = 0;
          ret = esp_spiffs_info(conf.partition_label, &total, &used);
          if (ret != ESP_OK) {
              ESP_LOGE(TAG_SPIFF, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
              ESP_ERROR_CHECK(esp_spiffs_format(conf.partition_label));
              
              return ESP_OK;
          } else {
              ESP_LOGI(TAG_SPIFF, "Partition size: total: %lu, used: %lu", total, used);
          }

      } else {
          ESP_LOGE(TAG_SPIFF, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
      }
      return ESP_OK;
  }
  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK) {
      ESP_LOGE(TAG_SPIFF, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
  } else {
      ESP_LOGI(TAG_SPIFF, "Partition size: total: %zu, used: %zu", total, used);
  }
 return ESP_OK;

}
