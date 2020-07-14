############### Add include ###################
list(APPEND ADD_INCLUDE "include"
     )
# list(APPEND ADD_PRIVATE_INCLUDE "")
###############################################

############ Add source files #################
# list(APPEND ADD_SRCS  "src/main.c")
append_srcs_dir(ADD_SRCS "src")

# aux_source_directory(src ADD_SRCS)
# list(REMOVE_ITEM COMPONENT_SRCS "src/test2.c")
###############################################

###### Add required/dependent components ######
list(APPEND ADD_REQUIREMENTS kendryte_sdk drivers utils)
###############################################

############ Add static libs ##################
# list(APPEND ADD_STATIC_LIB "lib/libtest.a")
###############################################


register_component()
