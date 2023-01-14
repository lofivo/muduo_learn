add_executable(AsyncLoggingTest AsyncLoggingTest.cc)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/logger/test)

target_link_libraries(AsyncLoggingTest mymuduo)