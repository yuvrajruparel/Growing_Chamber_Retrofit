#include "include/wifi.hpp"
#include "config.hpp"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "nvs_flash.h"

static const char *TAG = "wifi";
bool wifi_ll_init = false;
bool wifi_ready = false;

/**
 * Callback called whenever wifi is disconnected
 * Attempt to reconnect if so
 */
static void wifi_handle_disconnect(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data) {
  auto *event = (wifi_event_sta_disconnected_t *)event_data;

  if (event->reason == WIFI_REASON_ROAMING) {
    ESP_LOGI(TAG, "Wifi disconnect due to roaming.");
    return;
  }

  ESP_LOGI(TAG, "Wifi disconnected (%d), reconnecting...", event->reason);
  ESP_ERROR_CHECK_WITHOUT_ABORT(wifi::connect());
  wifi_ready = false;
}

/**
 * Callback when wifi is connected
 */
static void wifi_handle_connected(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data) {
  ESP_LOGI(TAG, "Connected.");
  // wifi_ready = true;
}

/**
 * Callback after IP configured
 */
static void wifi_handle_dhcp(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data) {
  auto event = (ip_event_got_ip_t *)event_data;

  ESP_LOGI(TAG, "IP address for interface %s: " IPSTR,
           esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));

  wifi_ready = true;
}

esp_err_t wifi::connect() {
  if (!wifi_ll_init) {
    // nonvolitile flash needs to be initialized before wifi connects
    nvs_flash_init();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_ll_init = true;
  }

  // use default wifi configuration
  const wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));

  ESP_ERROR_CHECK(esp_wifi_set_country_code("US", true));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  // print the mac address for debugging and so it can be registered
  uint8_t mac_base[6] = {0};
  esp_efuse_mac_get_default(mac_base);
  esp_read_mac(mac_base, ESP_MAC_WIFI_STA);

  uint8_t mac_local_base[6] = {0};
  uint8_t mac_uni_base[6] = {0};
  esp_derive_local_mac(mac_local_base, mac_uni_base);

  ESP_LOGI(TAG, "Local address: " MACSTR " | Uni Address: " MACSTR,
           MAC2STR(mac_local_base), MAC2STR(mac_uni_base));

  // configure the network to connect to
  wifi_config_t network_cfg = {.sta = {
                                   .ssid = WIFI_NAME,
                                   .password = WIFI_PASSWORD,
                                   .scan_method = WIFI_ALL_CHANNEL_SCAN,
                                   .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                               }};

  esp_wifi_set_config(WIFI_IF_STA, &network_cfg);

  // register callbacks
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                             WIFI_EVENT_STA_DISCONNECTED,
                                             &wifi_handle_disconnect, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &wifi_handle_dhcp, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_register(
      WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &wifi_handle_connected, nullptr));

  // perform connection
  ESP_LOGI(TAG, "Wifi configured. Connecting to AP...");
  ESP_ERROR_CHECK(esp_wifi_start());
  esp_err_t status = esp_wifi_connect();

  if (status != ESP_OK) {
    ESP_LOGE(TAG, "Failed to connect to wifi (%d): %s", status,
             esp_err_to_name(status));
  }

  ESP_LOGI(TAG, "Wifi setup complete.");

  return ESP_OK;
}

bool wifi::ready() { return wifi_ready; }
