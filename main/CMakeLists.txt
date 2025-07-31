file(GLOB_RECURSE SOURCES "*.cpp")
idf_component_register(SRCS ${SOURCES}
                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS "./Services")