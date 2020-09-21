############### Add include ###################
list(APPEND ADD_INCLUDE 
                        #esp8285
                        "network/esp8285"
                        "network/esp8285/http"
                        "network/esp8285/mqtt"
                        # "network/esp8285/qrcode"
                        "network/esp8285/wifi"
                        "network/esp8285/wifi/spi"
                        "network/esp8285/wifi/utility"
    )
# list(APPEND ADD_PRIVATE_INCLUDE "")
###############################################

############ Add source files #################
append_srcs_dir(ADD_SRCS "src")


if(CONFIG_NET_ENABLE)
    if(CONFIG_NET_ESP8285)
        message("-- Use ESP8285")
        FILE(GLOB_RECURSE NETWORK_SRC "network/esp8285/*.c")
    endif()
    list(APPEND ADD_SRCS ${NETWORK_SRC})
endif()

# aux_source_directory(src ADD_SRCS)
# list(REMOVE_ITEM COMPONENT_SRCS "src/test2.c")
###############################################

###### Add required/dependent components ######
list(APPEND ADD_REQUIREMENTS kendryte_sdk drivers)
###############################################

############ Add static libs ##################




register_component()
