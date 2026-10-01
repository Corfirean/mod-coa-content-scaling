# Included by modules/CMakeLists.txt. Registers module includes and tests with unit_tests.
target_include_directories(modules PUBLIC
    "${CMAKE_SOURCE_DIR}/modules/mod-coa-content-scaling/include"
)

set_property(GLOBAL APPEND PROPERTY ACORE_MODULE_TEST_INCLUDES
    "${CMAKE_SOURCE_DIR}/modules/mod-coa-content-scaling/include"
    "${CMAKE_SOURCE_DIR}/modules/mod-coa-content-scaling/src")

set_property(GLOBAL APPEND PROPERTY ACORE_MODULE_TEST_SOURCES
    "${CMAKE_SOURCE_DIR}/modules/mod-coa-content-scaling/src/ProgressionLayout.cpp"
    "${CMAKE_SOURCE_DIR}/modules/mod-coa-content-scaling/tests/ProgressionLayoutTest.cpp")

