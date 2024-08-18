#include "esp_http_server.h"
#include "esp_err.h"
#ifndef TAG
#define TAG "HTTP"
#endif

/**
 * URI handler for root (/) GET request.
 * Respond with the webpage to upload an image.
 */
esp_err_t get_handler(httpd_req_t * req);

/**
 * URI handler for root (/) POST request.
 * Read the binary image from the request.
 * Respond with an adequate page to show the result.
 */
esp_err_t post_handler(httpd_req_t * req);

/**
 * Start the http daemon in background on the default port
 */
esp_err_t init_http();

