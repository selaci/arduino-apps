include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_library(apps)

target_include_directories(apps
  PUBLIC
    "${CONAN_INCLUDE_DIRS}"
    "${PROJECT_SOURCE_DIR}/src")

target_link_libraries(apps PUBLIC "${CONAN_LIBS}")

target_sources(apps
  PUBLIC
    "${PROJECT_SOURCE_DIR}/src/sample_button.ino.cpp"
    "${PROJECT_SOURCE_DIR}/src/sample_led_matrix.ino.cpp"
    "${PROJECT_SOURCE_DIR}/src/sample_mpu6050.ino.cpp")
