file(GLOB_RECURSE SOURCES "*.cpp")
idf_component_register(SRCS ${SOURCES}
                    PRIV_REQUIRES spi_flash esp_driver_uart driver esp_adc esp_driver_i2c
                    INCLUDE_DIRS "./Services" "./Models" "./Enums")