#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

namespace pairwise {

struct Point {
    double x;
    double y;
};

enum class Geometry {
    standard,
    wraparound,
};

enum class Algorithm {
    naive,
    symmetry_optimized,
};

enum class Schedule {
    static_schedule,
    dynamic_schedule,
    guided_schedule,
};

struct AnalysisResult {
    std::vector<double> nearest_distances;
    std::vector<double> furthest_distances;
    double average_nearest = 0.0;
    double average_furthest = 0.0;
    std::uint64_t pair_evaluations = 0;
};

double euclidean_distance(const Point& first, const Point& second);
double wraparound_distance(const Point& first, const Point& second);

std::vector<Point> load_points_csv(const std::filesystem::path& path);
std::vector<Point> generate_points(std::size_t count, std::uint32_t seed);

AnalysisResult analyze(const std::vector<Point>& points,
                       Geometry geometry,
                       Algorithm algorithm,
                       Schedule schedule);

void write_distances_csv(const std::filesystem::path& path,
                         const std::vector<double>& distances);

std::string_view to_string(Geometry geometry);
std::string_view to_string(Algorithm algorithm);
std::string_view to_string(Schedule schedule);

}  // namespace pairwise
