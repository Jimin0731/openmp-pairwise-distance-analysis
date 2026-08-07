#include "pairwise_distance.hpp"

#include <omp.h>

#include <charconv>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

struct Options {
    std::optional<std::filesystem::path> input_path;
    std::optional<std::size_t> generated_count;
    std::uint32_t seed = 123U;
    pairwise::Geometry geometry = pairwise::Geometry::standard;
    pairwise::Algorithm algorithm = pairwise::Algorithm::naive;
    pairwise::Schedule schedule = pairwise::Schedule::static_schedule;
    std::optional<int> threads;
    std::optional<std::filesystem::path> output_prefix;
};

void print_usage(std::ostream& output) {
    output << "Usage:\n"
           << "  pairwise_distance_openmp (--generate N | --input points.csv) [options]\n\n"
           << "Options:\n"
           << "  --seed N                 Generator seed (default: 123)\n"
           << "  --geometry NAME          standard | wraparound (default: standard)\n"
           << "  --algorithm NAME         naive | optimized (default: naive)\n"
           << "  --schedule NAME          static | dynamic | guided (default: static)\n"
           << "  --threads N              OpenMP thread count\n"
           << "  --output-prefix PATH     Write PATH.nearest.csv and PATH.furthest.csv\n"
           << "  --help                    Show this message\n";
}

template <typename Integer>
Integer parse_integer(std::string_view value, std::string_view option) {
    Integer parsed{};
    const char* first = value.data();
    const char* last = first + value.size();
    const auto result = std::from_chars(first, last, parsed);
    if (result.ec != std::errc{} || result.ptr != last) {
        throw std::invalid_argument("Invalid value for " + std::string(option) + ": " +
                                    std::string(value));
    }
    return parsed;
}

std::string_view require_value(int argc, char** argv, int& index) {
    if (index + 1 >= argc) {
        throw std::invalid_argument("Missing value for " + std::string(argv[index]));
    }
    return argv[++index];
}

Options parse_options(int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        if (argument == "--help") {
            print_usage(std::cout);
            std::exit(0);
        }
        if (argument == "--input") {
            options.input_path = require_value(argc, argv, index);
        } else if (argument == "--generate") {
            options.generated_count =
                parse_integer<std::size_t>(require_value(argc, argv, index), argument);
        } else if (argument == "--seed") {
            options.seed =
                parse_integer<std::uint32_t>(require_value(argc, argv, index), argument);
        } else if (argument == "--threads") {
            options.threads = parse_integer<int>(require_value(argc, argv, index), argument);
        } else if (argument == "--output-prefix") {
            options.output_prefix = require_value(argc, argv, index);
        } else if (argument == "--geometry") {
            const std::string_view value = require_value(argc, argv, index);
            if (value == "standard") {
                options.geometry = pairwise::Geometry::standard;
            } else if (value == "wraparound" || value == "wrap") {
                options.geometry = pairwise::Geometry::wraparound;
            } else {
                throw std::invalid_argument("Unknown geometry: " + std::string(value));
            }
        } else if (argument == "--algorithm") {
            const std::string_view value = require_value(argc, argv, index);
            if (value == "naive") {
                options.algorithm = pairwise::Algorithm::naive;
            } else if (value == "optimized") {
                options.algorithm = pairwise::Algorithm::symmetry_optimized;
            } else {
                throw std::invalid_argument("Unknown algorithm: " + std::string(value));
            }
        } else if (argument == "--schedule") {
            const std::string_view value = require_value(argc, argv, index);
            if (value == "static") {
                options.schedule = pairwise::Schedule::static_schedule;
            } else if (value == "dynamic") {
                options.schedule = pairwise::Schedule::dynamic_schedule;
            } else if (value == "guided") {
                options.schedule = pairwise::Schedule::guided_schedule;
            } else {
                throw std::invalid_argument("Unknown schedule: " + std::string(value));
            }
        } else {
            throw std::invalid_argument("Unknown option: " + std::string(argument));
        }
    }

    if (options.input_path.has_value() == options.generated_count.has_value()) {
        throw std::invalid_argument("Choose exactly one of --input or --generate");
    }
    if (options.generated_count && *options.generated_count < 2) {
        throw std::invalid_argument("--generate must be at least 2");
    }
    if (options.threads && *options.threads < 1) {
        throw std::invalid_argument("--threads must be positive");
    }
    return options;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        if (options.threads) {
            omp_set_dynamic(0);
            omp_set_num_threads(*options.threads);
        }

        const std::vector<pairwise::Point> points = options.input_path
                                                        ? pairwise::load_points_csv(*options.input_path)
                                                        : pairwise::generate_points(
                                                              *options.generated_count, options.seed);

        const auto started = std::chrono::steady_clock::now();
        const auto result = pairwise::analyze(
            points, options.geometry, options.algorithm, options.schedule);
        const std::chrono::duration<double> elapsed =
            std::chrono::steady_clock::now() - started;

        std::cout << std::setprecision(17)
                  << "points=" << points.size() << '\n'
                  << "geometry=" << pairwise::to_string(options.geometry) << '\n'
                  << "algorithm=" << pairwise::to_string(options.algorithm) << '\n'
                  << "schedule=" << pairwise::to_string(options.schedule) << '\n'
                  << "threads=" << omp_get_max_threads() << '\n'
                  << "pair_evaluations=" << result.pair_evaluations << '\n'
                  << "average_nearest=" << result.average_nearest << '\n'
                  << "average_furthest=" << result.average_furthest << '\n'
                  << "elapsed_seconds_local_run=" << elapsed.count() << '\n';

        if (options.output_prefix) {
            pairwise::write_distances_csv(options.output_prefix->string() + ".nearest.csv",
                                          result.nearest_distances);
            pairwise::write_distances_csv(options.output_prefix->string() + ".furthest.csv",
                                          result.furthest_distances);
        }
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        print_usage(std::cerr);
        return 1;
    }
    return 0;
}
