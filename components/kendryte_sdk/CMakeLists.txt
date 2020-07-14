
################# Add include #################
list(APPEND ADD_INCLUDE 
                        "kendryte-standalone-sdk/lib/bsp/include"
                        "kendryte-standalone-sdk/lib/drivers/include"
                        "kendryte-standalone-sdk/lib/nncase/include"
                        
                        "kendryte-standalone-sdk/lib/utils/include/"
                        "kendryte-standalone-sdk/third_party/xtl/include"
    )
# list(APPEND ADD_PRIVATE_INCLUDE "include_private")
###############################################

############## Add source files ###############
append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/bsp")
append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/drivers")
append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/nncase")
append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/utils/include")


FILE(GLOB_RECURSE NNCASE_SRC
    "kendryte-standalone-sdk/lib/nncase/*.c"
    "kendryte-standalone-sdk/lib/nncase/*.cpp"
    )

append_srcs_dir(ADD_SRCS "src")
set(ADD_ASM_SRCS "kendryte-standalone-sdk/lib/bsp/crt.S")

list(APPEND ADD_SRCS  ${ADD_ASM_SRCS} ${NNCASE_SRC})

# list(REMOVE_ITEM COMPONENT_SRCS "src/test.c")

SET_PROPERTY(SOURCE ${ADD_ASM_SRCS} PROPERTY LANGUAGE C)
SET_SOURCE_FILES_PROPERTIES(${ADD_ASM_SRCS} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp -D __riscv64")
###############################################

###### Add required/dependent components ######
# list(APPEND ADD_REQUIREMENTS stdc++ m)
list(APPEND ADD_REQUIREMENTS  m)
###############################################

############ Add static libs ##################
# list(APPEND ADD_STATIC_LIB "libs/lib_mic.a"
#                            )
###############################################

list(APPEND ADD_DEFINITIONS -DCONFIG_LOG_LEVEL=${CONFIG_SDK_LOG_LEVEL}
                            -DNNCASE_TARGET=k210
                            -DTCB_SPAN_NO_EXCEPTIONS
                            -DTCB_SPAN_NO_CONTRACT_CHECKING
                            -DCONFIG_LOG_ENABLE=1
                            -DCONFIG_LOG_COLORS=1
                            -DLOG_KERNEL=1
                            -D__riscv64
                            )

register_component()

