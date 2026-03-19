
include_directories (
    "${CMAKE_CURRENT_LIST_DIR}"
    )


add_executable ( test_categories "${CMAKE_CURRENT_LIST_DIR}/test_categories.cpp" )
add_executable ( test_limits "${CMAKE_CURRENT_LIST_DIR}/test_limits.cpp" )
add_executable ( test_performance "${CMAKE_CURRENT_LIST_DIR}/test_performance.cpp" )

target_sources( test_categories
    PUBLIC
        "${LIB_DIR}/catch.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_jasterix.h"
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/test_cat001_1.1.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat001_flat_sacsic.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat002_1.0.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat004_1.4.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat010_0.31.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat010_0.24_sensis.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat019_1.3.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat020_1.5.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat021_0.26.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat021_2.1.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat030_7.0.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat034_1.26.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat048_1.15.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat048_1.23.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat062_1.12.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat062_1.16.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat063_1.0.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat065_1.3.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat247_1.2.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_cat252_7.0.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_encode.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_bounds.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_artas_md5.cpp"
)

target_sources( test_limits
    PUBLIC
        "${LIB_DIR}/catch.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_jasterix.h"
)


target_sources( test_performance
    PUBLIC
        "${LIB_DIR}/catch.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_jasterix.h"
)

target_link_libraries ( test_categories jasterix )
target_link_libraries ( test_limits jasterix )
target_link_libraries ( test_performance jasterix )

enable_testing()
set ( DEFINITION_PATH "${CMAKE_CURRENT_LIST_DIR}/../../definitions/" )
set ( DATA_PATH "${CMAKE_CURRENT_LIST_DIR}/" )

set ( TEST_FILENAME "/home/sk/data/test/test1/test.ff")

add_test(NAME TestCategories COMMAND test_categories --definition_path ${DEFINITION_PATH} --data_path ${DATA_PATH})
add_test(NAME TestLimits COMMAND test_limits --definition_path ${DEFINITION_PATH} --filename ${TEST_FILENAME})
add_test(NAME TestRAM COMMAND test_performance --definition_path ${DEFINITION_PATH} --filename ${TEST_FILENAME})
