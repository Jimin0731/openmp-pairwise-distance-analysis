#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <random>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <cmath> 
#include <algorithm> 
#include <omp.h>

using namespace std;

const string kDefaultCSVPath = "/Users/jiminbyun/Core Programming/data/100000 locations.csv";

struct Point {
    double x;
    double y;
};

double calcDist(const Point& p1, const Point& p2) {
    double dx = p1.x - p2.x;
    double dy = p1.y - p2.y;
    return std::sqrt(dx*dx + dy*dy);
}

double calcDistWrap(const Point& p1, const Point& p2) {
    double dx = std::abs(p1.x - p2.x);
    double dy = std::abs(p1.y - p2.y);

    if (dx > 0.5) dx = 1.0 - dx;
    
    if (dy > 0.5) dy = 1.0 - dy;

    return std::sqrt(dx*dx + dy*dy);
}

vector<Point> loadFromCSV(const string& filename) {
    vector<Point> points;
    ifstream file(filename);
    string line;

    if (!file.is_open()) {
        cerr << "Error: unable to open file -> " << filename << endl;
        return points;
    }

    while (getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        stringstream ss(line);
        string val;
        Point p;

        try {
            if (!getline(ss, val, ',')) {
                continue;
            }
            p.x = stod(val);
            if (!getline(ss, val, ',')) {
                continue;
            }
            p.y = stod(val);
        } catch (const invalid_argument&) {
            continue;
        } catch (const out_of_range&) {
            continue;
        }

        points.push_back(p);
    }
    
    cout << "Loaded " << points.size() << " points from " << filename << endl;
    return points;
}

vector<Point> generateRandom(int n) {
    vector<Point> points;
    points.reserve(n); 

    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    mt19937 gen(seed);
    uniform_real_distribution<double> dis(0.0, 1.0);

    for (int i = 0; i < n; ++i) {
        points.push_back({dis(gen), dis(gen)});
    }

    cout << "Generated " << n << " random points." << endl;
    return points;
}

struct AnalysisResult {
    vector<double> nearestDistances; 
    vector<double> furthestDistances; 
    double avgNearest;
    double avgFurthest;
};

AnalysisResult analyzePoints(const vector<Point>& points, double (*distFunc)(const Point&, const Point&)) {
    int n = points.size();
    AnalysisResult res;
    
    res.nearestDistances.resize(n);
    res.furthestDistances.resize(n);

    double totalNearest = 0.0;
    double totalFurthest = 0.0;
    #pragma omp parallel for reduction(+:totalNearest, totalFurthest) schedule(static)
    for (int i = 0; i < n; ++i) {
        double minDist = 100.0; 
        double maxDist = 0.0;

        for (int j = 0; j < n; ++j) {
            if (i == j) continue;

            double d = distFunc(points[i], points[j]);

            if (d < minDist) minDist = d;
            if (d > maxDist) maxDist = d;
        }

        res.nearestDistances[i] = minDist;
        res.furthestDistances[i] = maxDist;

        totalNearest += minDist;
        totalFurthest += maxDist;
    }

    res.avgNearest = totalNearest / n;
    res.avgFurthest = totalFurthest / n;

    return res;
}

void saveToFile(const string& filename, const vector<double>& data) {
    ofstream outFile(filename);
    if (!outFile.is_open()) {
        cerr << "Error: Can't generate the file -> " << filename << endl;
        return;
    }
    
    outFile.precision(6);
    outFile << fixed;

    for (double val : data) {
        outFile << val << endl;
    }
    cout << "Saved " << data.size() << " values to " << filename << endl;
}

int main() {
    vector<Point> data;
    int choice;

    cout << "Choose data input method (1: CSV file, 2: random generation): ";
    cin >> choice;

    if (choice == 1) {
        cout << "Using default CSV path (" << kDefaultCSVPath << ")." << endl;
        data = loadFromCSV(kDefaultCSVPath);
    } 
    else if (choice == 2) {
        int n;
        cout << "Enter number of points to generate (e.g., 100000): ";
        cin >> n;
        data = generateRandom(n);
    } 
    else {
        cout << "Invalid choice." << endl;
        return 1;
    }

    if (!data.empty()) {
        cout << "\n--- First 5 points ---" << endl;
        for (size_t i = 0; i < 5 && i < data.size(); ++i) {
            cout << "Point " << i << ": (" << data[i].x << ", " << data[i].y << ")" << endl;
        }
    }
    Point pA = {0.1, 0.1};
    Point pB = {0.9, 0.1};
    
    cout << "\n--- Distance function test ---" << endl;
    cout << "Point A(0.1, 0.1) <-> Point B(0.9, 0.1)" << endl;
    cout << "Standard distance: " << calcDist(pA, pB) << " (Expected value: 0.8)" << endl;
    cout << "Wrap distance: " << calcDistWrap(pA, pB) << " (Expected value: 0.2)" << endl;

    cout << "\n--- Entire data analysis (Naive Algorithm) ---" << endl;
    cout << "Number of data: " << data.size() << " " << endl;

    auto start = chrono::high_resolution_clock::now();
    AnalysisResult resStd = analyzePoints(data, calcDist);
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> diff = end - start;

    cout << "[Standard Geometry]" << endl;
    cout << "  Mean the nearest distance: " << resStd.avgNearest << endl;
    cout << "  Mean the furthest distance:    " << resStd.avgFurthest << endl;
    cout << "  Time spent:           " << diff.count() << " s" << endl;

    cout << "\nSaving Standard Geometry results..." << endl;
    saveToFile("standard_nearest.csv", resStd.nearestDistances);
    saveToFile("standard_furthest.csv", resStd.furthestDistances);

    start = chrono::high_resolution_clock::now();
    AnalysisResult resWrap = analyzePoints(data, calcDistWrap);
    end = chrono::high_resolution_clock::now();
    diff = end - start;

    cout << "[Wraparound Geometry]" << endl;
    cout << "  Mean the nearest distance: " << resWrap.avgNearest << endl;
    cout << "  Mean the furthes distance:    " << resWrap.avgFurthest << endl;
    cout << "  Time spent:           " << diff.count() << " s" << endl;

    cout << "\nSaving Wraparound Geometry results..." << endl;
    saveToFile("wrap_nearest.csv", resWrap.nearestDistances);
    saveToFile("wrap_furthest.csv", resWrap.furthestDistances);

    cout << "\nAll tasks completed." << endl;

    return 0;
}
