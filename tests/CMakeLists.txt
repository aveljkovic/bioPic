cmake_minimum_required(VERSION 3.24)

function(biopic_copy_c_dll target_name)
  add_custom_command(
    TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:biopic_c>
            $<TARGET_FILE_DIR:${target_name}>
    COMMENT "Copy biopic_c shared library beside ${target_name}")
endfunction()

add_library(biopic_test_support STATIC support/test_images.cpp)
target_include_directories(biopic_test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/support)
target_link_libraries(biopic_test_support PUBLIC biopic_core ${OpenCV_LIBS})

function(biopic_add_test test_name)
  add_executable(${test_name} ${ARGN})
  target_link_libraries(${test_name} PRIVATE biopic_core biopic_test_support GTest::gtest_main)
  biopic_set_target_warnings(${test_name})
  add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

biopic_add_test(test_distance unit/test_distance.cpp)
biopic_add_test(test_fingerprint_codec unit/test_fingerprint_codec.cpp)
biopic_add_test(test_hasher unit/test_hasher.cpp)
biopic_add_test(test_image_decoder unit/test_image_decoder.cpp)
biopic_add_test(test_golden_vectors golden/test_golden_vectors.cpp)
biopic_add_test(test_determinism golden/test_determinism.cpp)
biopic_add_test(test_transformations golden/test_transformations.cpp)
biopic_add_test(test_c_abi unit/test_c_abi.cpp)
if(BIOPIC_BUILD_C_ABI)
  target_link_libraries(test_c_abi PRIVATE biopic_c)
  biopic_copy_c_dll(test_c_abi)

  enable_language(C)
  add_executable(biopic_c_smoke c_smoke/smoke.c)
  target_include_directories(biopic_c_smoke PRIVATE ${CMAKE_SOURCE_DIR}/bindings/c/include)
  target_link_libraries(biopic_c_smoke PRIVATE biopic_c)
  biopic_copy_c_dll(biopic_c_smoke)
  add_test(NAME biopic_c_smoke COMMAND biopic_c_smoke)
endif()
