ADD_SUBDIRECTORY(util)
ADD_SUBDIRECTORY(math)
ADD_SUBDIRECTORY(chemistry)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mpqc_config.h DESTINATION include/mpqc COMPONENT mpqc-config)