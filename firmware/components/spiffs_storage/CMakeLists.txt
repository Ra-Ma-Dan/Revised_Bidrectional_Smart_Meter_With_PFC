idf_component_register(SRCS "spiffs_storage.c"
                    INCLUDE_DIRS "include"
                    REQUIRES spiffs meter_http_server)
