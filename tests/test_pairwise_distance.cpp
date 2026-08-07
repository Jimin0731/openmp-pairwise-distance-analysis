#include "pairwise_distance.hpp"

#include <omp.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void check_close(double actual, double expected, double tolerance, const std::string& message) {
    check(std::abs(actual - expected) <= tolerance,
          message + " (actual=" + std::to_string(actual) +
              ", expected=" + std::to_string(expected) + ")");
}

void check_vector_close(const std::vector<double>& actual,
                        const std::vector<double>& expected,
                        double tolerance,
                        const std::string& message) {
    check(actual.size() == expected.size(), message + " size");
    if (actual.size() != expected.size()) {
        return;
    }
    for (std::size_t index = 0; index < actual.size(); ++index) {
        check_close(actual[index], expected[index], tolerance,
                    message + " at index " + std::to_string(index));
    }
}

void expect_exception(const std::function<void()>& action, const std::string& message) {
    try {
        action();
        check(false, message + " did not throw");
    } catch (const std::exception&) {
        // Expected.
    }
}

void test_distance_functions() {
    const pairwise::Point first{0.1, 0.1};
    const pairwise::Point second{0.9, 0.1};
    check_close(pairwise::euclidean_distance(first, second), 0.8, 1e-12,
                "standard boundary-crossing distance");
    check_close(pairwise::wraparound_distance(first, second), 0.2, 1e-12,
                "wraparound boundary-crossing distance");

    check_close(pairwise::euclidean_distance({0.0, 0.0}, {1.0, 1.0}),
                std::sqrt(2.0), 1e-12, "opposite standard corners");
    check_close(pairwise::wraparound_distance({0.0, 0.0}, {1.0, 1.0}),
                0.0, 1e-12, "opposite wraparound corners");
}

void test_known_datasets() {
    const std::vector<pairwise::Point> two_points{{0.1, 0.1}, {0.9, 0.1}};
    const auto standard = pairwise::analyze(two_points, pairwise::Geometry::standard,
                                            pairwise::Algorithm::naive,
                                            pairwise::Schedule::static_schedule);
    check_vector_close(standard.nearest_distances, {0.8, 0.8}, 1e-12,
                       "two-point standard nearest");
    check_vector_close(standard.furthest_distances, {0.8, 0.8}, 1e-12,
                       "two-point standard furthest");
    check(standard.pair_evaluations == 2U, "two-point naive evaluation count");

    const auto wrap = pairwise::analyze(two_points, pairwise::Geometry::wraparound,
                                        pairwise::Algorithm::symmetry_optimized,
                                        pairwise::Schedule::dynamic_schedule);
    check_vector_close(wrap.nearest_distances, {0.2, 0.2}, 1e-12,
                       "two-point wraparound nearest");
    check_vector_close(wrap.furthest_distances, {0.2, 0.2}, 1e-12,
                       "two-point wraparound furthest");
    check(wrap.pair_evaluations == 1U, "two-point optimized evaluation count");

    const std::vector<pairwise::Point> line_points{{0.1, 0.1}, {0.4, 0.1}, {0.9, 0.1}};
    const auto line_standard = pairwise::analyze(
        line_points, pairwise::Geometry::standard, pairwise::Algorithm::naive,
        pairwise::Schedule::guided_schedule);
    check_vector_close(line_standard.nearest_distances, {0.3, 0.3, 0.5}, 1e-12,
                       "known line standard nearest");
    check_vector_close(line_standard.furthest_distances, {0.8, 0.5, 0.8}, 1e-12,
                       "known line standard furthest");

    const std::vector<pairwise::Point> duplicates{{0.2, 0.3}, {0.2, 0.3}, {0.7, 0.8}};
    const auto duplicate_result = pairwise::analyze(
        duplicates, pairwise::Geometry::standard,
        pairwise::Algorithm::symmetry_optimized, pairwise::Schedule::guided_schedule);
    check_close(duplicate_result.nearest_distances[0], 0.0, 1e-12,
                "duplicate point zero nearest distance");
    check_close(duplicate_result.nearest_distances[1], 0.0, 1e-12,
                "second duplicate point zero nearest distance");
    check_close(duplicate_result.nearest_distances[2], std::sqrt(0.5), 1e-12,
                "duplicate dataset third-point nearest distance");
}

void compare_all_variants() {
    const auto points = pairwise::generate_points(73, 20251203U);
    const std::vector<pairwise::Schedule> schedules{
        pairwise::Schedule::static_schedule,
        pairwise::Schedule::dynamic_schedule,
        pairwise::Schedule::guided_schedule,
    };
    const std::vector<pairwise::Geometry> geometries{
        pairwise::Geometry::standard,
        pairwise::Geometry::wraparound,
    };

    for (const auto geometry : geometries) {
        const auto reference = pairwise::analyze(
            points, geometry, pairwise::Algorithm::naive,
            pairwise::Schedule::static_schedule);
        for (const auto schedule : schedules) {
            for (const auto algorithm : {pairwise::Algorithm::naive,
                                         pairwise::Algorithm::symmetry_optimized}) {
                const auto candidate = pairwise::analyze(points, geometry, algorithm, schedule);
                const std::string label = std::string(pairwise::to_string(geometry)) + " " +
                                          std::string(pairwise::to_string(algorithm)) + " " +
                                          std::string(pairwise::to_string(schedule));
                check_vector_close(candidate.nearest_distances,
                                   reference.nearest_distances, 1e-12,
                                   label + " nearest equivalence");
                check_vector_close(candidate.furthest_distances,
                                   reference.furthest_distances, 1e-12,
                                   label + " furthest equivalence");
                check_close(candidate.average_nearest, reference.average_nearest,
                            1e-12, label + " average nearest equivalence");
                check_close(candidate.average_furthest, reference.average_furthest,
                            1e-12, label + " average furthest equivalence");
            }
        }
    }
}

void test_seeded_generation() {
    const auto first = pairwise::generate_points(16, 42U);
    const auto second = pairwise::generate_points(16, 42U);
    const auto different = pairwise::generate_points(16, 43U);
    check(first.size() == second.size(), "deterministic generation size");
    for (std::size_t index = 0; index < first.size(); ++index) {
        check(first[index].x == second[index].x && first[index].y == second[index].y,
              "same seed reproduces point " + std::to_string(index));
    }
    check(first.front().x != different.front().x || first.front().y != different.front().y,
          "different seed changes generated data");
    expect_exception([] { static_cast<void>(pairwise::generate_points(1, 42U)); },
                     "generation with fewer than two points");
}

void test_csv_parsing() {
    const auto base = std::filesystem::temp_directory_path() /
                      ("pairwise-distance-test-" + std::to_string(omp_get_thread_num()));
    std::filesystem::create_directories(base);
    const auto valid = base / "valid.csv";
    {
        std::ofstream output(valid);
        output << "x,y\n0.1,0.2\n 0.9 , 1.0 \n";
    }
    const auto points = pairwise::load_points_csv(valid);
    check(points.size() == 2U, "CSV parser reads header and two points");
    check_close(points[1].x, 0.9, 1e-12, "CSV parser x value");
    check_close(points[1].y, 1.0, 1e-12, "CSV parser y value");

    const auto invalid = base / "invalid.csv";
    {
        std::ofstream output(invalid);
        output << "0.1,not-a-number\n";
    }
    expect_exception([&] { static_cast<void>(pairwise::load_points_csv(invalid)); },
                     "invalid CSV numeric value");

    const auto outside = base / "outside.csv";
    {
        std::ofstream output(outside);
        output << "0.1,1.1\n";
    }
    expect_exception([&] { static_cast<void>(pairwise::load_points_csv(outside)); },
                     "CSV coordinate outside unit square");

    const auto extra_column = base / "extra-column.csv";
    {
        std::ofstream output(extra_column);
        output << "0.1,0.2,0.3\n";
    }
    expect_exception([&] { static_cast<void>(pairwise::load_points_csv(extra_column)); },
                     "CSV with an extra column");
    std::filesystem::remove_all(base);
}

void test_invalid_analysis_inputs() {
    expect_exception(
        [] {
            static_cast<void>(pairwise::analyze(
                {{0.1, 0.2}}, pairwise::Geometry::standard,
                pairwise::Algorithm::naive, pairwise::Schedule::static_schedule));
        },
        "analysis with one point");
    expect_exception(
        [] {
            static_cast<void>(pairwise::analyze(
                {{0.1, 0.2}, {1.1, 0.3}}, pairwise::Geometry::standard,
                pairwise::Algorithm::naive, pairwise::Schedule::static_schedule));
        },
        "analysis with out-of-range point");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 2) {
        const int threads = std::stoi(argv[1]);
        omp_set_dynamic(0);
        omp_set_num_threads(threads);
    }

    test_distance_functions();
    test_known_datasets();
    compare_all_variants();
    test_seeded_generation();
    test_csv_parsing();
    test_invalid_analysis_inputs();

    if (failures != 0) {
        std::cerr << failures << " test assertion(s) failed\n";
        return 1;
    }
    std::cout << "All scientific correctness tests passed with up to "
              << omp_get_max_threads() << " OpenMP thread(s).\n";
    return 0;
}
