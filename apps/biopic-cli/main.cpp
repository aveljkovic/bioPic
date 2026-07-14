#include "cli_commands.hpp"
#include "cli_common.hpp"

#include "biopic/cli/exit_codes.hpp"

#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    if (argc < 2) {
        biopic::cli::print_root_usage();
        return biopic::cli::kExitError;
    }

    const std::string_view command = argv[1];
    if (command == "hash") {
        return biopic::cli::run_hash(argc, argv);
    }
    if (command == "compare") {
        return biopic::cli::run_compare(argc, argv);
    }
    if (command == "scan") {
        return biopic::cli::run_scan(argc, argv);
    }
    if (command == "classify") {
        return biopic::cli::run_classify(argc, argv);
    }
    if (command == "evaluate") {
        return biopic::cli::run_evaluate(argc, argv);
    }
    if (command == "benchmark") {
        return biopic::cli::run_benchmark(argc, argv);
    }
    if (command == "database") {
        return biopic::cli::run_database(argc, argv);
    }
    if (command == "model") {
        return biopic::cli::run_model(argc, argv);
    }
    if (command == "config") {
        return biopic::cli::run_config(argc, argv);
    }
    if (command == "doctor") {
        return biopic::cli::run_doctor(argc, argv);
    }
    if (command == "version") {
        return biopic::cli::run_version(argc, argv);
    }
    if (command == "--help" || command == "-h" || command == "help") {
        biopic::cli::print_root_usage();
        return 0;
    }

    std::cerr << "Unknown command: " << command << '\n';
    biopic::cli::print_root_usage();
    return biopic::cli::kExitError;
}
