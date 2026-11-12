idf_component_register(SRCS "main.cpp" "wifi.cpp" "humidity.cpp" "request.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_wifi esp_adc driver nvs_flash esp_event esp_http_client json)
