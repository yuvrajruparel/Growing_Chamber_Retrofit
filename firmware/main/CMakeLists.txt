idf_component_register(SRCS "main.cpp" ./modes/humidify.cpp ./modes/dehumidify.cpp ./modes/idle.cpp ./modes/machine.cpp telemetry/request.cpp ./telemetry/wifi.cpp ./humidity.cpp
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_wifi esp_adc driver nvs_flash esp_event esp_http_client json fan)
