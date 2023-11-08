idf_component_register(SRCS "drv_stdio.c" 
                    INCLUDE_DIRS "." 
                    REQUIRES    "driver" 
                                "drv_stream" 
                                "drv_console"
                                "app_socket" 
                                      )
                 

