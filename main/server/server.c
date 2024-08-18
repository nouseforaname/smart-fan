#include "server.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "../ota/ota.h" 
#include "esp_system.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "driver/gpio.h"

#define GPIO_SPEED_1 GPIO_NUM_12// Venti Gross => GPIO_NUM_4
#define GPIO_SPEED_2 GPIO_NUM_14// Venti Gross => GPIO_NUM_5
#define GPIO_SPEED_3 GPIO_NUM_16// Venti Gross => GPIO_NUM_2
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_SPEED_1) | (1ULL<<GPIO_SPEED_2) | (1ULL<<GPIO_SPEED_3))

const int gpio = 2;
bool pinState = false;

static const char * UPLOAD_SUCCESS = "Successfully uploaded image";
static const char * RESTART_SUCCESS = "Will restart now. Flashing new image";
static const char * UPLOAD_FAIL = "Failed to upload image";
static const char *TAG_OTA = "simple_ota";
extern esp_ota_firm_t ota_firm;

#define QUOTE(...) #__VA_ARGS__
  char *responseThing = QUOTE(
{
  "id": "urn:uuid:548f95f6-ac5b-4169-b1c7-a043eabce9e9",
  "@context": "https://iot.mozilla.org/schemas",
  "securityDefinitions": {
    "nosec_sc": {
      "scheme": "nosec"
    }
  },
  "title": "venti",
  "description": "Venti-Speed",
  "properties": {
    "speed": {
      "@type": "LevelProperty",
      "type": "integer",
      "title": "speed",
      "links": [{ "rel":"property", "href": "/things/1/properties/speed"}],
      "minimum": 0,
      "maximum": 3
    }
  },
  "links": [
    {
      "href": "/things/1/properties",
      "rel": "properties",
      "mediaType": "application/json"
    }
    ,{
      "href": "ws://localhost/",
      "rel": "alternate",
      "mediaType": "application/json"
    }
  ]
}
);  
esp_err_t speed_web_things_handler (httpd_req_t * req) {
  char *wrappedThings = malloc( strlen(responseThing)+2);
  sprintf(wrappedThings, "[%s]", responseThing);    
  httpd_resp_set_status(req, "200 OK");
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, wrappedThings, strlen(wrappedThings));
  ESP_LOGI(TAG, "things called");
  return ESP_OK;
}
esp_err_t speed_things_handler (httpd_req_t * req) {
   
  httpd_resp_set_status(req, "200 OK");
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, responseThing, strlen(responseThing));
  ESP_LOGI(TAG, "speed thing called");
  return ESP_OK;
}

esp_err_t restart_handler(httpd_req_t * req)
{
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, RESTART_SUCCESS, strlen(RESTART_SUCCESS));

    ESP_LOGI(TAG, "Prepare to restart system in 1!");
    esp_restart();

    return ESP_OK;
}
char *readFileBytes( char* path) {
  FILE *settingsFile = fopen(path, "r");
  struct stat buf;
  int fileNo = fileno(settingsFile);
  fstat(fileNo, &buf);
  if (settingsFile == NULL) {
      char *msg = "Failed to open file for reading";
      ESP_LOGE(TAG, "%c",*msg);
      return msg; 
  }
  char *buffer = malloc(buf.st_size + 1);
  buffer[buf.st_size] = '\0'; 
  fread(buffer, buf.st_size, 1, settingsFile); 
  fclose(settingsFile);
  return buffer; 
}
esp_err_t get_speed_handler(httpd_req_t * req)
{
  char *buffer = readFileBytes("/spiffs/speed.json");
  httpd_resp_set_status(req, "200 OK");
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, buffer, strlen(buffer));
  free(buffer);
  return ESP_OK;
}
esp_err_t get_config_handler(httpd_req_t * req)
{
  char *jsonString = readFileBytes("/spiffs/settings.json");
  httpd_resp_set_status(req, "200 OK");
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, jsonString, strlen(jsonString));
  free(jsonString);
  return ESP_OK;
}
void allPinsLow(){
   gpio_set_level(GPIO_SPEED_1, 0);
   gpio_set_level(GPIO_SPEED_2, 0);
   gpio_set_level(GPIO_SPEED_3, 0);
}
esp_err_t setPinHigh(int PINON){
   allPinsLow(); 
   gpio_set_level(PINON, 1);
   return ESP_OK;
}
esp_err_t put_speed_handler(httpd_req_t * req)
{
    ESP_LOGI(TAG, "Reading provided config");

    size_t ret = 0, remaining = req->content_len;
    char settingsBytes[remaining + 1];
    settingsBytes[remaining] = '\0';
    ESP_LOGI(TAG, "Expecting bytes: %u", remaining);

    while (remaining > 0) {
        // Read data from request
        ret = httpd_req_recv(req, settingsBytes, remaining < sizeof(settingsBytes) ? remaining : sizeof(settingsBytes));

        // Retry receiving if timeout occurred
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        } else if (ret <= 0) {
            return ESP_FAIL;
        }
        remaining -= ret;
    }
    ESP_LOGI(TAG, "Parsing json config");
    cJSON *rootDoc = cJSON_Parse(settingsBytes);

    if (!rootDoc) {
      ESP_LOGI(TAG, "Couldn't parse json");
      return ESP_ERR_HTTPD_INVALID_REQ;
    }
    cJSON *element = rootDoc->child;
    ESP_LOGI(TAG, "traversing JSON values");
    while ( element != NULL ) {
      char *name = element->string;
      int value = element->valueint;
      ESP_LOGI(TAG,"name: %s, value: %d", name, value);
      element = element->next;
      if ( strcmp( name, "speed" ) != 0 ) { 
        ESP_LOGI(TAG, "wrong element in speed json, found %s, expected \"value\"",name);
        return ESP_ERR_HTTPD_INVALID_REQ;
      }
      switch(value) {
          case 0:
              ESP_LOGI(TAG_OTA, "SETTING OFF");
              allPinsLow();
              break;
          case 1:
              ESP_LOGI(TAG_OTA, "SETTING SPEED 1");
              allPinsLow();
              setPinHigh(GPIO_SPEED_1);
              break;
          case 2:
              ESP_LOGI(TAG_OTA, "SETTING SPEED 2");
              allPinsLow();
              setPinHigh(GPIO_SPEED_2);
              break;
          case 3:
              ESP_LOGI(TAG_OTA, "SETTING SPEED 3");
              allPinsLow();
              setPinHigh(GPIO_SPEED_3);
              break;
          default:
              ESP_LOGI(TAG_OTA, "SETTING Unknown Value");
              allPinsLow();
              break;
      }
      
    }
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(TAG, "Opening settings file");
    FILE *settingsFile = fopen("/spiffs/speed.json", "w");
    if (settingsFile == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        
        httpd_resp_set_status(req, "501 Internal Server Error");
        char *msg = "error opening file for write";
        httpd_resp_send(req, msg , strlen(msg));
        return ESP_ERR_FLASH_OP_FAIL;
    }
    
    fputs(settingsBytes,settingsFile);
    fclose(settingsFile);

    ESP_LOGI(TAG, "File written");
    ESP_LOGI(TAG, "contents: %s", settingsBytes);

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, settingsBytes, strlen(settingsBytes));

    return ESP_OK;
}

esp_err_t post_config_handler(httpd_req_t * req)
{
    ESP_LOGI(TAG, "Reading provided config");

    size_t ret = 0, remaining = req->content_len;
    char settingsBytes[remaining];

    ESP_LOGI(TAG, "Expecting bytes: %zu", remaining);

    while (remaining > 0) {
        // Read data from request
        ret = httpd_req_recv(req, settingsBytes, remaining < sizeof(settingsBytes) ? remaining : sizeof(settingsBytes));

        // Retry receiving if timeout occurred
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        } else if (ret <= 0) {
            return ESP_FAIL;
        }
        remaining -= ret;
    }
    ESP_LOGI(TAG, "Parsing json config");
    cJSON *rootDoc = cJSON_Parse(settingsBytes);

    if (!rootDoc) {
      ESP_LOGI(TAG, "Couldn't parse json");
      return ESP_ERR_HTTPD_INVALID_REQ;
    }
    cJSON *element = rootDoc->child;

    ESP_LOGI(TAG, "traversing JSON values");
    while ( element != NULL ) {

      ESP_LOGI(TAG,"name: %s", element->string);
      ESP_LOGI(TAG,"value: %s", element->valuestring);
      element = element->next;
    }


    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(TAG, "Opening settings file");
    FILE *settingsFile = fopen("/spiffs/settings.json", "w");
    if (settingsFile == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_ERR_FLASH_OP_FAIL;
    }

    fprintf(settingsFile, "%s", cJSON_Print(rootDoc));
    fclose(settingsFile);

    ESP_LOGI(TAG, "File written");

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, "OK", strlen("OK"));

    return ESP_OK;
}

esp_err_t post_handler(httpd_req_t * req)
{
    ESP_LOGI(TAG, "Received file upload %u bytes", req->content_len);

    // Basic check
    if (req->method != HTTP_POST) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, UPLOAD_FAIL, strlen(UPLOAD_FAIL));
        return ESP_OK;
    }

    char buf[100];
    size_t ret = 0, remaining = req->content_len;
    while (remaining > 0) {
        // Read data from request
        ret = httpd_req_recv(req, buf, remaining < sizeof(buf) ? remaining : sizeof(buf));

        // Retry receiving if timeout occurred
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        } else if (ret <= 0) {
            return ESP_FAIL;
        }

        // Write read bytes into the OTA partition
        if (write_ota(&ota_firm, buf, ret) != ESP_OK) {
            return ESP_FAIL;
        }

        remaining -= ret;
    }

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_send(req, UPLOAD_SUCCESS, strlen(UPLOAD_SUCCESS));

    // End ota_firm and restart
    if (end_ota(&ota_firm) != ESP_OK) {
        ESP_LOGE(TAG, "Error: end_ota");
        return ESP_FAIL;
    }

    esp_restart();
    return ESP_OK;
}
esp_err_t init_speed_pins(){
  gpio_config_t io_conf;
  //disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  //set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  //bit mask of the pins that you want to set,e.g.GPIO15/16
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  //disable pull-down mode
  io_conf.pull_down_en = 0;
  //disable pull-up mode
  io_conf.pull_up_en = 0;
  //configure GPIO with the given settings
  gpio_config(&io_conf);
  allPinsLow();
  return ESP_OK;
}

esp_err_t init_http()
{
    ESP_ERROR_CHECK(init_speed_pins());
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server on default port
    ESP_LOGI(TAG, "Starting HTTP server on port: '%d'", config.server_port);
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    // URI handler for speed 
    httpd_uri_t speed_web_things = {
      .uri       = "/",
      .method    = HTTP_GET,
      .handler   = speed_web_things_handler,
      .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &speed_web_things);
    httpd_uri_t speed_thing = {
      .uri       = "/things/1",
      .method    = HTTP_GET,
      .handler   = speed_things_handler,
      .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &speed_thing);

    httpd_uri_t get_properties = {
        .uri       = "/things/1/properties",
        .method    = HTTP_GET,
        .handler   = get_speed_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t get_speed_property = {
        .uri       = "/things/1/properties/speed",
        .method    = HTTP_GET,
        .handler   = get_speed_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &get_properties);
    httpd_register_uri_handler(server, &get_speed_property);

    httpd_uri_t put_speed = {
        .uri       = "/things/1/properties/speed",
        .method    = HTTP_PUT,
        .handler   = put_speed_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &put_speed);

    // URI handler for config
    httpd_uri_t post_conf = {
        .uri       = "/config",
        .method    = HTTP_POST,
        .handler   = post_config_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &post_conf);

    // URI handler for config
    httpd_uri_t get_config = {
        .uri       = "/config",
        .method    = HTTP_GET,
        .handler   = get_config_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &get_config);
    // URI handle for upload
    httpd_uri_t upload = {
        .uri       = "/",
        .method    = HTTP_POST,
        .handler   = post_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &upload);

    return ESP_OK;
}
