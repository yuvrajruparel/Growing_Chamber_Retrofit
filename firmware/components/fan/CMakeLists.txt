idf_component_register(SRCS fan.cpp watchdog.cpp
                    INCLUDE_DIRS "include"
                    REQUIRES esp_timer esp_driver_ledc)
