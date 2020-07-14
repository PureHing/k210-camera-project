list(APPEND ADD_INCLUDE 
                        #camera
                        "camera/include"
                        "camera/ov2640/include"
                        "camera/ov5640/include"
                        "camera/gc0328/include"
                        #lcd
                        "lcd/include"
                        #w25q_flash
                        "w25q_flash/include"
                        #sd card
                        "sd_card/include"
                        )

if(CONFIG_ENABLE_CAMERA)
    append_srcs_dir(ADD_SRCS "camera/src")

    if(CONFIG_CAMERA_GC0328)
        append_srcs_dir(ADD_SRCS "camera/gc0328/src")
    endif()
    if(CONFIG_CAMERA_OV5640)
        append_srcs_dir(ADD_SRCS "camera/ov5640/src")
    endif()
    if(CONFIG_CAMERA_OV2640)
        append_srcs_dir(ADD_SRCS "camera/ov2640/src")
    endif()
endif()

if(CONFIG_ENABLE_LCD)
    append_srcs_dir(ADD_SRCS "lcd/src")
endif()

if(CONFIG_ENABLE_FLASH)
    append_srcs_dir(ADD_SRCS "w25q_flash/src")
endif()
append_srcs_dir(ADD_SRCS "w25q_flash/src")
if(CONFIG_ENABLE_SD_CARD)
    append_srcs_dir(ADD_SRCS "sd_card/src")
endif()
list(APPEND ADD_REQUIREMENTS kendryte_sdk)
register_component()
