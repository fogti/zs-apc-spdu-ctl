include_directories("${PROJECT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" ${NETSNMP_INCLUDE_DIRS})

add_executable(zs-apc-spdu-ctl args.cxx conf.cxx confent.cxx confent_outlets.cxx main.cxx
                               snmp/parse.cxx snmp/zs_snmp.cxx)

target_link_libraries(zs-apc-spdu-ctl Threads::Threads LowlevelZS::lowlevelzs ${NETSNMP_LIBRARIES})
install(TARGETS zs-apc-spdu-ctl DESTINATION "${CMAKE_INSTALL_BINDIR}")
