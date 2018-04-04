include_directories("${CMAKE_CURRENT_SOURCE_DIR}" ${NETSNMP_INCLUDE_DIRS})

add_executable(zs-apc-spdu-ctl args.cxx conf.cxx confent.cxx main.cxx
                               snmp/parse.cxx snmp/zs_snmp.cxx)

target_link_libraries(zs-apc-spdu-ctl ${CMAKE_THREAD_LIBS_INIT} ${NETSNMP_LIBRARIES})
install(TARGETS zs-apc-spdu-ctl DESTINATION "${INSTALL_BIN_DIR}")
