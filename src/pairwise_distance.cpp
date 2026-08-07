#include "pairwise_distance.hpp"

#include <omp.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace pairwise {
namespace {

using DistanceFunction = double (*)(const Point&, const Point&);

class RuntimeScheduleGuard {
  public:
    explicit RuntimeScheduleGuard(Schedule schedule) {
        omp_get_schedule(&previous_kind_, &previous_chunk_);
        omp_sched_t kind = omp_sched_static;
        switch (schedule) {
            case Schedule::static_schedule:
                kind = omp_sched_static;
                break;
            case Schedule::dynamic_schedule:
                kind = omp_sched_dynamic;
                break;
            case Schedule::guided_schedule:
                kind = omp_sched_guided;
                break;
        }
        omp_set_schedule(kind, 0);
    }

    ~RuntimeScheduleGuard() { omp_set_schedule(previous_kind_, previous_chunk_); }

    RuntimeScheduleGuard(const RuntimeScheduleGuard&) = delete;
    RuntimeScheduleGuard& operator=(const RuntimeScheduleGuard&) = delete;

  private:
    omp_sched_t previous_kind_{};
    int previous_chunk_ = 0;
};

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

double parse_coordinate(const std::string& text,
                        const std::filesystem::path& path,
                        std::size_t line_number) {
    const std::string cleaned = trim(text);
    std::size_t parsed = 0;
    double value = 0.0;
    try {
        value = std::stod(cleaned, &parsed);
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid numeric value in " + path.string() +
                                 " at line " + std::to_string(line_number));
    }
    if (parsed != cleaned.size() || !std::isfinite(value)) {
        throw std::runtime_error("Invalid numeric value in " + path.string() +
                                 " at line " + std::to_string(line_number));
    }
    if (value < 0.0 || value > 1.0) {
        throw std::runtime_error("Coordinate outside [0,1] in " + path.string() +
                                 " at line " + std::to_string(line_number));
    }
    return value;
}

void validate_points(const std::vector<Point>& points) {
    if (points.size() < 2) {
        throw std::invalid_argument("At least two points are required");
    }
    if (points.size() > static_cast<std::size_t>(
                            std::numeric_limits<std::int64_t>::max())) {
        throw std::length_error("Point count exceeds the supported OpenMP loop range");
    }
    for (const auto& point : points) {
        if (!std::isfinite(point.x) || !std::isfinite(point.y) || point.x < 0.0 ||
            point.x > 1.0 || point.y < 0.0 || point.y > 1.0) {
            throw std::invalid_argument("Every point must be finite and inside [0,1] x [0,1]");
        }
    }
}

DistanceFunction distance_function(Geometry geometry) {
    return geometry == Geometry::standard ? euclidean_distance : wraparound_distance;
}

AnalysisResult analyze_naive(const std::vector<Point>& points,
                             DistanceFunction distance,
                             Schedule schedule) {
    const auto count = static_cast<std::int64_t>(points.size());
    AnalysisResult result;
    result.nearest_distances.resize(points.size());
    result.furthest_distances.resize(points.size());

    double total_nearest = 0.0;
    double total_furthest = 0.0;
    const RuntimeScheduleGuard schedule_guard(schedule);

    // Runtime scheduling is set immediately above to one of OpenMP static,
    // dynamic, or guided. Each outer iteration performs the same N-1 distances.
#pragma omp parallel for reduction(+ : total_nearest, total_furthest) schedule(runtime)
    for (std::int64_t i = 0; i < count; ++i) {
        double nearest = std::numeric_limits<double>::infinity();
        double furthest = 0.0;
        for (std::int64_t j = 0; j < count; ++j) {
            if (i == j) {
                continue;
            }
            const double value = distance(points[static_cast<std::size_t>(i)],
                                          points[static_cast<std::size_t>(j)]);
            nearest = std::min(nearest, value);
            furthest = std::max(furthest, value);
        }
        result.nearest_distances[static_cast<std::size_t>(i)] = nearest;
        result.furthest_distances[static_cast<std::size_t>(i)] = furthest;
        total_nearest += nearest;
        total_furthest += furthest;
    }

    result.average_nearest = total_nearest / static_cast<double>(count);
    result.average_furthest = total_furthest / static_cast<double>(count);
    result.pair_evaluations = static_cast<std::uint64_t>(count) *
                              static_cast<std::uint64_t>(count - 1);
    return result;
}

AnalysisResult analyze_optimized(const std::vector<Point>& points,
                                 DistanceFunction distance,
                                 Schedule schedule) {
    const auto count = static_cast<std::int64_t>(points.size());
    const std::size_t point_count = points.size();
    const int thread_count = omp_get_max_threads();
    if (point_count > std::numeric_limits<std::size_t>::max() /
                          static_cast<std::size_t>(thread_count)) {
        throw std::length_error("Thread-local storage size overflow");
    }

    const std::size_t local_count = point_count * static_cast<std::size_t>(thread_count);
    std::vector<double> local_minima(local_count,
                                     std::numeric_limits<double>::infinity());
    std::vector<double> local_maxima(local_count, 0.0);
    const RuntimeScheduleGuard schedule_guard(schedule);

    // The j > i loop evaluates each symmetric pair once. Per-thread arrays let
    // both endpoints be updated without atomics or races.
#pragma omp parallel
    {
        const auto thread = static_cast<std::size_t>(omp_get_thread_num());
        const std::size_t offset = thread * point_count;

#pragma omp for schedule(runtime)
        for (std::int64_t i = 0; i < count - 1; ++i) {
            const auto first = static_cast<std::size_t>(i);
            for (std::int64_t j = i + 1; j < count; ++j) {
                const auto second = static_cast<std::size_t>(j);
                const double value = distance(points[first], points[second]);

                local_minima[offset + first] =
                    std::min(local_minima[offset + first], value);
                local_maxima[offset + first] =
                    std::max(local_maxima[offset + first], value);
                local_minima[offset + second] =
                    std::min(local_minima[offset + second], value);
                local_maxima[offset + second] =
                    std::max(local_maxima[offset + second], value);
            }
        }
    }

    AnalysisResult result;
    result.nearest_distances.resize(point_count);
    result.furthest_distances.resize(point_count);
    double total_nearest = 0.0;
    double total_furthest = 0.0;

#pragma omp parallel for reduction(+ : total_nearest, total_furthest) schedule(static)
    for (std::int64_t i = 0; i < count; ++i) {
        const auto point = static_cast<std::size_t>(i);
        double nearest = std::numeric_limits<double>::infinity();
        double furthest = 0.0;
        for (int thread = 0; thread < thread_count; ++thread) {
            const std::size_t index = static_cast<std::size_t>(thread) * point_count + point;
            nearest = std::min(nearest, local_minima[index]);
            furthest = std::max(furthest, local_maxima[index]);
        }
        result.nearest_distances[point] = nearest;
        result.furthest_distances[point] = furthest;
        total_nearest += nearest;
        total_furthest += furthest;
    }

    result.average_nearest = total_nearest / static_cast<double>(count);
    result.average_furthest = total_furthest / static_cast<double>(count);
    result.pair_evaluations = static_cast<std::uint64_t>(count) *
                              static_cast<std::uint64_t>(count - 1) / 2U;
    return result;
}

}  // namespace

double euclidean_distance(const Point& first, const Point& second) {
    return std::hypot(first.x - second.x, first.y - second.y);
}

double wraparound_distance(const Point& first, const Point& second) {
    const double direct_x = std::abs(first.x - second.x);
    const double direct_y = std::abs(first.y - second.y);
    const double dx = std::min(direct_x, 1.0 - direct_x);
    const double dy = std::min(direct_y, 1.0 - direct_y);
    return std::hypot(dx, dy);
}

std::vector<Point> load_points_csv(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Unable to open CSV file: " + path.string());
    }

    std::vector<Point> points;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (trim(line).empty()) {
            continue;
        }
        if (std::count(line.begin(), line.end(), ',') != 1) {
            throw std::runtime_error("Expected exactly two CSV columns in " + path.string() +
                                     " at line " + std::to_string(line_number));
        }

        std::istringstream row(line);
        std::string x_text;
        std::string y_text;
        if (!std::getline(row, x_text, ',') || !std::getline(row, y_text, ',')) {
            throw std::runtime_error("Expected two CSV columns in " + path.string() +
                                     " at line " + std::to_string(line_number));
        }

        if (points.empty() && trim(x_text) == "x" && trim(y_text) == "y") {
            continue;
        }
        points.push_back({parse_coordinate(x_text, path, line_number),
                          parse_coordinate(y_text, path, line_number)});
    }

    if (points.empty()) {
        throw std::runtime_error("CSV file contains no points: " + path.string());
    }
    return points;
}

std::vector<Point> generate_points(std::size_t count, std::uint32_t seed) {
    if (count < 2) {
        throw std::invalid_argument("At least two generated points are required");
    }
    std::mt19937 generator(seed);
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    std::vector<Point> points;
    points.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        points.push_back({distribution(generator), distribution(generator)});
    }
    return points;
}

AnalysisResult analyze(const std::vector<Point>& points,
                       Geometry geometry,
                       Algorithm algorithm,
                       Schedule schedule) {
    validate_points(points);
    const DistanceFunction distance = distance_function(geometry);
    return algorithm == Algorithm::naive ? analyze_naive(points, distance, schedule)
                                         : analyze_optimized(points, distance, schedule);
}

void write_distances_csv(const std::filesystem::path& path,
                         const std::vector<double>& distances) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Unable to write CSV file: " + path.string());
    }
    output << std::setprecision(17);
    for (const double distance : distances) {
        output << distance << '\n';
    }
}

std::string_view to_string(Geometry geometry) {
    return geometry == Geometry::standard ? "standard" : "wraparound";
}

std::string_view to_string(Algorithm algorithm) {
    return algorithm == Algorithm::naive ? "naive" : "optimized";
}

std::string_view to_string(Schedule schedule) {
    switch (schedule) {
        case Schedule::static_schedule:
            return "static";
        case Schedule::dynamic_schedule:
            return "dynamic";
        case Schedule::guided_schedule:
            return "guided";
    }
    return "unknown";
}

}  // namespace pairwise
