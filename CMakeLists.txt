idf_build_get_property(project_dir PROJECT_DIR)

# Initialize an empty list to hold conditional requirements
set(conditionally_required_components "")

if(EXISTS "${project_dir}/components/drv_stream")
    list(APPEND conditionally_required_components "drv_stream")
endif()

if(EXISTS "${project_dir}/components/drv_console")
    list(APPEND conditionally_required_components "drv_console")
endif()

if(EXISTS "${project_dir}/components/app_socket_stdio")
    list(APPEND conditionally_required_components "app_socket_stdio")
endif()

idf_component_register(SRCS "drv_stdio.c" 
                    INCLUDE_DIRS "." 
                    REQUIRES    "driver" 
                                ${conditionally_required_components}
                                      )
                 

