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
target_link_libraries(biopic_test_support PUBLIC biopic_core)

function(biopic_add_test test_name)
  add_executable(${test_name} ${ARGN})
  target_link_libraries(${test_name} PRIVATE biopic_core biopic_test_support GTest::gtest_main)
  biopic_set_target_warnings(${test_name})
  add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

biopic_add_test(test_distance unit/test_distance.cpp)
biopic_add_test(test_fingerprint_codec unit/test_fingerprint_codec.cpp)
biopic_add_test(test_hasher unit/test_hasher.cpp)
biopic_add_test(test_uniform_colors unit/test_uniform_colors.cpp)
biopic_add_test(test_image_decoder unit/test_image_decoder.cpp)
biopic_add_test(test_golden_vectors golden/test_golden_vectors.cpp)
biopic_add_test(test_determinism golden/test_determinism.cpp)
biopic_add_test(test_transformations golden/test_transformations.cpp)
biopic_add_test(test_fingerprint_store unit/test_fingerprint_store.cpp)
biopic_add_test(test_sqlite_fingerprint_store unit/test_sqlite_fingerprint_store.cpp)
biopic_add_test(test_classifier unit/test_classifier.cpp)
biopic_add_test(test_classifier_config unit/test_classifier_config.cpp)
biopic_add_test(test_scanner unit/test_scanner.cpp)
target_compile_definitions(
  test_scanner
  PRIVATE BIOPIC_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
biopic_add_test(test_model unit/test_model.cpp)
biopic_add_test(test_output_mapping unit/test_output_mapping.cpp)
biopic_add_test(test_preprocessing unit/test_preprocessing.cpp)
biopic_add_test(test_tensor_contract unit/test_tensor_contract.cpp)
biopic_add_test(test_onnx_runtime unit/test_onnx_runtime.cpp)
target_compile_definitions(
  test_onnx_runtime
  PRIVATE BIOPIC_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
target_compile_definitions(
  test_classifier_config
  PRIVATE BIOPIC_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
if(BIOPIC_BUILD_CLI)
  set(CLI_CLASSIFY_FIXTURE_PNG ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/cli_classify.png)
  set(CLI_CLASSIFIER_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/classifier.conf)

  add_test(NAME test_cli_classify COMMAND $<TARGET_FILE:biopic-cli> classify
                                          ${CLI_CLASSIFY_FIXTURE_PNG} --config
                                          ${CLI_CLASSIFIER_CONFIG})
  set_tests_properties(
    test_cli_classify
    PROPERTIES
      PASS_REGULAR_EXPRESSION
      "Label: channel_r[\r\n]+Confidence: 0.[0-9]+[\r\n]+Detection result: not detected")

  add_test(NAME test_cli_scan COMMAND $<TARGET_FILE:biopic-cli> scan ${CLI_CLASSIFY_FIXTURE_PNG}
                                        --config ${CLI_CLASSIFIER_CONFIG})
  set_tests_properties(
    test_cli_scan
    PROPERTIES
      PASS_REGULAR_EXPRESSION
      "BioPic Scan[\r\n]+[\r\n]+Image:[\r\n]+.*[\r\n]+[\r\n]+Fingerprint:[\r\n]+  Algorithm: biopic[\r\n]+  Version: 1[\r\n]+  Status: generated[\r\n]+[\r\n]+Database:[\r\n]+  Status: not configured[\r\n]+  Match: no match[\r\n]+  Distance: 0.000000[\r\n]+[\r\n]+Classifier:[\r\n]+  Status: available[\r\n]+  Label: channel_r[\r\n]+  Confidence: 0.[0-9]+[\r\n]+  Detection: false[\r\n]+[\r\n]+Decision:[\r\n]+  ALLOW")

  add_test(NAME test_cli_scan_without_classifier COMMAND $<TARGET_FILE:biopic-cli> scan
                                                        ${CLI_CLASSIFY_FIXTURE_PNG})
  set_tests_properties(
    test_cli_scan_without_classifier
    PROPERTIES
      ENVIRONMENT "BIOPIC_CLASSIFIER_CONFIG="
      PASS_REGULAR_EXPRESSION
      "BioPic Scan[\r\n]+[\r\n]+Image:[\r\n]+.*[\r\n]+[\r\n]+Fingerprint:[\r\n]+  Algorithm: biopic[\r\n]+  Version: 1[\r\n]+  Status: generated[\r\n]+[\r\n]+Database:[\r\n]+  Status: not configured[\r\n]+  Match: no match[\r\n]+  Distance: 0.000000[\r\n]+[\r\n]+Classifier:[\r\n]+  Status: not configured[\r\n]+[\r\n]+Decision:[\r\n]+  ALLOW")

  set(CLI_PERSISTENCE_DATABASE ${CMAKE_CURRENT_BINARY_DIR}/cli_persistence_test.db)
  add_test(
    NAME test_cli_database_persistence
    COMMAND ${CMAKE_COMMAND} -DBIOPIC_CLI=$<TARGET_FILE:biopic-cli>
            -DBIOPIC_IMAGE=${CLI_CLASSIFY_FIXTURE_PNG}
            -DBIOPIC_DATABASE=${CLI_PERSISTENCE_DATABASE} -P
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_cli_database_persistence.cmake)
  set_tests_properties(test_cli_database_persistence PROPERTIES RUN_SERIAL TRUE)
endif()
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
