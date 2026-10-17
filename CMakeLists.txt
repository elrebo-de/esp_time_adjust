idf_component_register( SRCS "time_adjust.cpp"
                        INCLUDE_DIRS "include"
                        REQUIRES "driver"
                                 "esp_timer"
                                 "esp_event"
                      )

