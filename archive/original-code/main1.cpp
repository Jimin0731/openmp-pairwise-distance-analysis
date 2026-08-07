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
#include <iomanip> 

using namespace std;

const string kDefaultCSVPath = "/Users/jiminbyun/Core Programming/data/100000 locations.csv";
//it's my own path, you can change it to your own path

struct Point {
    double x;
    double y;
};

// Standard Eulidean distance
double calcDist(const Point& p1, const Point& p2) {
    double dx = p1.x - p2.x;
    double dy = p1.y - p2.y;
    return std::sqrt(dx*dx + dy*dy);
}

// Wraparound distance
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
        if (line.empty()) continue;
        stringstream ss(line);
        string val;
        Point p;
        try {
            if (!getline(ss, val, ',')) continue;
            p.x = stod(val);
            if (!getline(ss, val, ',')) continue;
            p.y = stod(val);
        } catch (...) { continue; }
        points.push_back(p);
    }
    cout << "Loaded " << points.size() << " points from " << filename << endl;
    return points;
}

// Generate n random points in [0,1) x [0,1)
vector<Point> generateRandom(int n) {
    vector<Point> points;
    points.reserve(n); 
    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    mt19937 gen(seed);
    uniform_real_distribution<double> dis(0.0, 1.0);
    for (int i = 0; i < n; ++i) points.push_back({dis(gen), dis(gen)});
    cout << "Generated " << n << " random points." << endl;
    return points;
}

struct AnalysisResult {
    vector<double> nearestDistances; 
    vector<double> furthestDistances; 
    double avgNearest;
    double avgFurthest;
};

void writeDistancesToFile(const vector<double>& distances, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }
    for (double d : distances) {
        file << d << "\n";
    }
    file.close();
    cout << "Saved file: " << filename << endl;
}

//Naive algorithm

AnalysisResult analyzeNaiveStatic(const vector<Point>& points, double (*distFunc)(const Point&, const Point&)) {
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

//Dynamic schedule
AnalysisResult analyzeNaiveDynamic(const vector<Point>& points, double (*distFunc)(const Point&, const Point&)) {
    int n = points.size();
    AnalysisResult res;
    res.nearestDistances.resize(n);
    res.furthestDistances.resize(n);

    double totalNearest = 0.0;
    double totalFurthest = 0.0;
    
    #pragma omp parallel for reduction(+:totalNearest, totalFurthest) schedule(dynamic)
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

//Guided schedule
AnalysisResult analyzeNaiveGuided(const vector<Point>& points, double (*distFunc)(const Point&, const Point&)) {
    int n = points.size();
    AnalysisResult res;
    res.nearestDistances.resize(n);
    res.furthestDistances.resize(n);

    double totalNearest = 0.0;
    double totalFurthest = 0.0;
    
    #pragma omp parallel for reduction(+:totalNearest, totalFurthest) schedule(guided)
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

//Optimized algorithm
AnalysisResult analyzeOptimizedStatic(const vector<Point>& points, double (*distFunc)(const Point&, const Point&)) {
    int n = points.size();
    int max_threads = omp_get_max_threads();
    
    vector<double> localMin(n * max_threads, 100.0);
    vector<double> localMax(n * max_threads, 0.0);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int offset = tid * n;

        #pragma omp for schedule(static)
        for (int i = 0; i < n - 1; ++i) {
            for (int j = i + 1; j < n; ++j) { 
                double d = distFunc(points[i], points[j]);

                if (d < localMin[offset + i]) localMin[offset + i] = d;
                if (d > localMax[offset + i]) localMax[offset + i] = d;

                if (d < localMin[offset + j]) localMin[offset + j] = d;
                if (d > localMax[offset + j]) localMax[offset + j] = d;
            }
        }
    }

    AnalysisResult res;
    res.nearestDistances.resize(n);
    res.furthestDistances.resize(n);
    double totalNearest = 0.0;
    double totalFurthest = 0.0;

    #pragma omp parallel for reduction(+:totalNearest, totalFurthest)
    for (int i = 0; i < n; ++i) {
        double finalMin = 100.0;
        double finalMax = 0.0;
        for (int t = 0; t < max_threads; ++t) {
            if (localMin[t * n + i] < finalMin) finalMin = localMin[t * n + i];
            if (localMax[t * n + i] > finalMax) finalMax = localMax[t * n + i];
        }
        res.nearestDistances[i] = finalMin;
        res.furthestDistances[i] = finalMax;
        totalNearest += finalMin;
        totalFurthest += finalMax;
    }
    res.avgNearest = totalNearest / n;
    res.avgFurthest = totalFurthest / n;
    return res;
}

//Dynamic schedule
AnalysisResult analyzeOptimizedDynamic(const vector<Point>& points, double (*distFunc)(const Point&, const Point&)) {
    int n = points.size();
    int max_threads = omp_get_max_threads();
    vector<double> localMin(n * max_threads, 100.0);
    vector<double> localMax(n * max_threads, 0.0);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int offset = tid * n;

        #pragma omp for schedule(dynamic)
        for (int i = 0; i < n - 1; ++i) {
            for (int j = i + 1; j < n; ++j) { 
                double d = distFunc(points[i], points[j]);
                if (d < localMin[offset + i]) localMin[offset + i] = d;
                if (d > localMax[offset + i]) localMax[offset + i] = d;
                if (d < localMin[offset + j]) localMin[offset + j] = d;
                if (d > localMax[offset + j]) localMax[offset + j] = d;
            }
        }
    }

    AnalysisResult res;
    res.nearestDistances.resize(n);
    res.furthestDistances.resize(n);
    double totalNearest = 0.0;
    double totalFurthest = 0.0;

    #pragma omp parallel for reduction(+:totalNearest, totalFurthest)
    for (int i = 0; i < n; ++i) {
        double finalMin = 100.0;
        double finalMax = 0.0;
        for (int t = 0; t < max_threads; ++t) {
            if (localMin[t * n + i] < finalMin) finalMin = localMin[t * n + i];
            if (localMax[t * n + i] > finalMax) finalMax = localMax[t * n + i];
        }
        res.nearestDistances[i] = finalMin;
        res.furthestDistances[i] = finalMax;
        totalNearest += finalMin;
        totalFurthest += finalMax;
    }
    res.avgNearest = totalNearest / n;
    res.avgFurthest = totalFurthest / n;
    return res;
}

//Guided schedule
AnalysisResult analyzeOptimizedGuided(const vector<Point>& points, double (*distFunc)(const Point&, const Point&)) {
    int n = points.size();
    int max_threads = omp_get_max_threads();
    vector<double> localMin(n * max_threads, 100.0);
    vector<double> localMax(n * max_threads, 0.0);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int offset = tid * n;

        #pragma omp for schedule(guided)
        for (int i = 0; i < n - 1; ++i) {
            for (int j = i + 1; j < n; ++j) { 
                double d = distFunc(points[i], points[j]);
                if (d < localMin[offset + i]) localMin[offset + i] = d;
                if (d > localMax[offset + i]) localMax[offset + i] = d;
                if (d < localMin[offset + j]) localMin[offset + j] = d;
                if (d > localMax[offset + j]) localMax[offset + j] = d;
            }
        }
    }

    AnalysisResult res;
    res.nearestDistances.resize(n);
    res.furthestDistances.resize(n);
    double totalNearest = 0.0;
    double totalFurthest = 0.0;

    #pragma omp parallel for reduction(+:totalNearest, totalFurthest)
    for (int i = 0; i < n; ++i) {
        double finalMin = 100.0;
        double finalMax = 0.0;
        for (int t = 0; t < max_threads; ++t) {
            if (localMin[t * n + i] < finalMin) finalMin = localMin[t * n + i];
            if (localMax[t * n + i] > finalMax) finalMax = localMax[t * n + i];
        }
        res.nearestDistances[i] = finalMin;
        res.furthestDistances[i] = finalMax;
        totalNearest += finalMin;
        totalFurthest += finalMax;
    }
    res.avgNearest = totalNearest / n;
    res.avgFurthest = totalFurthest / n;
    return res;
}

void printTableHeader() {
    cout << "----------------------------------------------------------------" << endl;
    cout << "| Method   | Schedule | Avg Nearest | Avg Furthest | Time (s)  |" << endl;
    cout << "----------------------------------------------------------------" << endl;
}

void printTableRow(const string& method, const string& schedule, double avgNearest, double avgFurthest, double time) {
    cout << " | " << left << setw(8) << method 
         << " | " << setw(8) << schedule 
         << " | " << right << setw(11) << fixed << setprecision(5) << avgNearest
         << " | " << setw(12) << avgFurthest
         << " | " << setw(9) << setprecision(4) << time << " |" << endl;
}

void printTableFooter() {
    cout << "----------------------------------------------------------------" << endl;
}

// Run Naive Algorithm
void runNaiveAlgorithm(const vector<Point>& data, double (*distFunc)(const Point&, const Point&), const string& geomType) {
    cout << "\n NAIVE ALGORITHM - " << geomType << endl;
    printTableHeader();
    
    auto start = chrono::high_resolution_clock::now();
    AnalysisResult resStatic = analyzeNaiveStatic(data, distFunc);
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> diffStatic = end - start;
    printTableRow("Naive", "Static", resStatic.avgNearest, resStatic.avgFurthest, diffStatic.count());

    start = chrono::high_resolution_clock::now();
    AnalysisResult resDynamic = analyzeNaiveDynamic(data, distFunc);
    end = chrono::high_resolution_clock::now();
    chrono::duration<double> diffDynamic = end - start;
    printTableRow("Naive", "Dynamic", resDynamic.avgNearest, resDynamic.avgFurthest, diffDynamic.count());

    start = chrono::high_resolution_clock::now();
    AnalysisResult resGuided = analyzeNaiveGuided(data, distFunc);
    end = chrono::high_resolution_clock::now();
    chrono::duration<double> diffGuided = end - start;
    printTableRow("Naive", "Guided", resGuided.avgNearest, resGuided.avgFurthest, diffGuided.count());
    
    printTableFooter();

    string filePrefix = (geomType.find("STANDARD") != string::npos) ? "standard" : "wrap";
    
    writeDistancesToFile(resStatic.nearestDistances, filePrefix + "_nearest.csv");
    writeDistancesToFile(resStatic.furthestDistances, filePrefix + "_furthest.csv");

}

// Run Optimized Algorithm
void runOptimizedAlgorithm(const vector<Point>& data, double (*distFunc)(const Point&, const Point&), const string& geomType) {
    cout << "\n OPTIMIZED ALGORITHM - " << geomType << endl;
    printTableHeader();

    auto start = chrono::high_resolution_clock::now();
    AnalysisResult resStatic = analyzeOptimizedStatic(data, distFunc);
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> diffStatic = end - start;
    printTableRow("Optimized", "Static", resStatic.avgNearest, resStatic.avgFurthest, diffStatic.count());

    start = chrono::high_resolution_clock::now();
    AnalysisResult resDynamic = analyzeOptimizedDynamic(data, distFunc);
    end = chrono::high_resolution_clock::now();
    chrono::duration<double> diffDynamic = end - start;
    printTableRow("Optimized", "Dynamic", resDynamic.avgNearest, resDynamic.avgFurthest, diffDynamic.count());

    start = chrono::high_resolution_clock::now();
    AnalysisResult resGuided = analyzeOptimizedGuided(data, distFunc);
    end = chrono::high_resolution_clock::now();
    chrono::duration<double> diffGuided = end - start;
    printTableRow("Optimized", "Guided", resGuided.avgNearest, resGuided.avgFurthest, diffGuided.count());

    printTableFooter();

    string filePrefix = (geomType.find("STANDARD") != string::npos) ? "standard" : "wrap";
    
    writeDistancesToFile(resStatic.nearestDistances, filePrefix + "_nearest_optimized.csv");
    writeDistancesToFile(resStatic.furthestDistances, filePrefix + "_furthest_optimized.csv");

}

int main() {
    vector<Point> data;
    int choice;
    
    cout << "Choose data input method (1: CSV file, 2: random generation): ";
    cin >> choice;

    if (choice == 1) {
        data = loadFromCSV(kDefaultCSVPath);
    } else if (choice == 2) {
        int n;
        cout << "Enter number of points (e.g., 10000): "; 
        cin >> n;
        data = generateRandom(n);
    } else {
        cout << "Invalid choice!" << endl;
        return 1;
    }

    if (data.empty()) return 1;

    int algoChoice;
    cout << "\nChoose algorithm:" << endl;
    cout << "1: Naive only" << endl;
    cout << "2: Optimized only" << endl;
    cout << "3: Both" << endl;
    cout << "Enter choice: ";
    cin >> algoChoice;

    int geomChoice;
    cout << "\nChoose geometry:" << endl;
    cout << "1: Standard geometry" << endl;
    cout << "2: Wraparound geometry" << endl;
    cout << "3: Both" << endl;
    cout << "Enter choice: ";
    cin >> geomChoice;

    cout << "PERFORMANCE TEST" << endl;
    cout << "Number of Points: " << data.size() << endl;
    cout << "Max Threads: " << omp_get_max_threads() << endl;

    if (geomChoice == 1 || geomChoice == 3) {
        // Standard geometry
        if (algoChoice == 1 || algoChoice == 3) {
            runNaiveAlgorithm(data, calcDist, "STANDARD GEOMETRY");
        }
        if (algoChoice == 2 || algoChoice == 3) {
            runOptimizedAlgorithm(data, calcDist, "STANDARD GEOMETRY");
        }
    }

    if (geomChoice == 2 || geomChoice == 3) {
        // Wraparound geometry
        if (algoChoice == 1 || algoChoice == 3) {
            runNaiveAlgorithm(data, calcDistWrap, "WRAPAROUND GEOMETRY");
        }
        if (algoChoice == 2 || algoChoice == 3) {
            runOptimizedAlgorithm(data, calcDistWrap, "WRAPAROUND GEOMETRY");
        }
    }

    cout << "TEST COMPLETED" << endl;

    return 0;
}