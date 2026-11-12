#include "cJSON.h"
#include "config.hpp"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "include/wifi.hpp"
#include <stdio.h>
#include <string.h>

static const char *TAG = "http_log";

/**
 * @brief Handles HTTP client events.
 * * This function logs events from the HTTP client, such as connection status,
 * headers, and data received. It's useful for debugging.
 */
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
  case HTTP_EVENT_ERROR:
    ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key,
             evt->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    if (!esp_http_client_is_chunked_response(evt->client)) {
      // Log the response data for debugging
      ESP_LOGI(TAG, "Response body: %.*s", evt->data_len, (char *)evt->data);
    }
    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
    break;
  case HTTP_EVENT_REDIRECT:
    ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
    break;
  }
  return ESP_OK;
}

/**
 * @brief Creates a JSON payload and sends it via HTTP POST.
 *
 * @param real_ua The first integer value.
 * @param real_raw The second integer value.
 * @param sp_mv The third integer value.
 * @param sp_raw The fourth integer value.
 *
 * @note This function assumes WiFi is already connected.
 * You MUST update the `url` in esp_http_client_config_t.
 */
void wifi::post_sensor_data(float real_ua, int real_raw, float sp_mv,
                            int sp_raw) {
  cJSON *root = cJSON_CreateObject();
  if (root == NULL) {
    ESP_LOGE(TAG, "Failed to create cJSON root object.");
    return;
  }

  float real_humidity = (real_ua - 4000.0f) / 16000.0f;
  float setpoint_humidity = sp_mv / 10000.0f;

  // Add integer values to the JSON object
  cJSON_AddNumberToObject(root, "real_ua", real_ua);
  cJSON_AddNumberToObject(root, "real_raw", real_raw);
  cJSON_AddNumberToObject(root, "sp_mv", sp_mv);
  cJSON_AddNumberToObject(root, "sp_raw", sp_raw);
  cJSON_AddNumberToObject(root, "humidity_rH", real_humidity);
  cJSON_AddNumberToObject(root, "setpoint_rH", setpoint_humidity);

  // Convert the cJSON object to a string
  const char *json_payload = cJSON_PrintUnformatted(root);
  if (json_payload == NULL) {
    ESP_LOGE(TAG, "Failed to print cJSON object to string.");
    cJSON_Delete(root);
    return;
  }

  ESP_LOGI(TAG, "JSON Payload: %s", json_payload);

  esp_http_client_config_t config = {
      .url = HTTP_ENDPOINT,
      .event_handler = _http_event_handler,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);

  // Set headers and method
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_header(client, "Content-Type", "application/json");

  // Set the JSON string as the request body
  esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

  // Perform the HTTP request
  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %" PRId64,
             esp_http_client_get_status_code(client),
             esp_http_client_get_content_length(client));
  } else {
    ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
  }

  // Cleanup
  esp_http_client_cleanup(client);
  cJSON_Delete(root);
  free((void *)json_payload); // cJSON_Print allocates memory that must be freed
}
