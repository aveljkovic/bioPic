#pragma once

namespace biopic::cli {

int run_hash(int argc, char** argv);
int run_compare(int argc, char** argv);
int run_scan(int argc, char** argv);
int run_evaluate(int argc, char** argv);
int run_benchmark(int argc, char** argv);
int run_database(int argc, char** argv);
int run_model(int argc, char** argv);
int run_config(int argc, char** argv);
int run_doctor(int argc, char** argv);
int run_version(int argc, char** argv);

// Backward compatibility with Milestone 9 tests.
int run_classify(int argc, char** argv);

} // namespace biopic::cli
