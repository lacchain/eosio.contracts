add_contract(lacchain.system lacchain.system ${CMAKE_CURRENT_SOURCE_DIR}/src/lacchain.system.cpp)

target_include_directories(lacchain.system
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(lacchain.system
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/lacchain.system.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/lacchain.system.contracts.md @ONLY )

target_compile_options( lacchain.system PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
