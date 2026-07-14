cmake_minimum_required(VERSION 3.24)

add_executable(bench_hash bench_hash.cpp)
target_link_libraries(bench_hash PRIVATE biopic_core biopic_test_support benchmark::benchmark)
biopic_set_target_warnings(bench_hash)

add_executable(bench_similarity_search bench_similarity_search.cpp)
target_link_libraries(bench_similarity_search PRIVATE biopic_core biopic_test_support
                                                         benchmark::benchmark)
biopic_set_target_warnings(bench_similarity_search)
