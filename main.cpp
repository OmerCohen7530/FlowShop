#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <numeric>
#include <iomanip>
#include <stdexcept>
#include "FlowShopOutsource.cpp"

static void printJobList(const std::vector<flowshop::Job>& jobs,
                         const char* emptyText,
                         const char* separator) {
    if (jobs.empty()) {
        std::cout << emptyText;
        return;
    }

    for (size_t i = 0; i < jobs.size(); ++i) {
        std::cout << "J" << (jobs[i].id + 1);
        if (i + 1 < jobs.size()) std::cout << separator;
    }
}

static void printNaiveResult(const flowshop_ext::NaiveResult& result, int U) {
    std::cout << "\n=== RESULTS ===" << std::endl;
    std::cout << "Best Objective (Naive): " << result.objective << std::endl;
    std::cout << "Outsourcing cost: " << result.outsourcingCost << " / " << U << std::endl;

    std::cout << "In-house order: ";
    printJobList(result.inhouseOrder, "(none)", " -> ");
    std::cout << std::endl;

    std::cout << "Outsourced jobs: ";
    printJobList(result.outsourced, "(none)", ", ");
    std::cout << std::endl;
}

static void printDPResult(const flowshop_ext::NaiveResult& result, int U) {
    std::cout << "\n=== RESULTS ===" << std::endl;
    std::cout << "Best Objective (DP): " << result.objective << std::endl;
    std::cout << "Outsourcing cost: " << result.outsourcingCost << " / " << U << std::endl;

    std::cout << "In-house order: ";
    printJobList(result.inhouseOrder, "(none)", " -> ");
    std::cout << std::endl;

    std::cout << "Outsourced jobs: ";
    printJobList(result.outsourced, "(none)", ", ");
    std::cout << std::endl;
}

struct RandomInstance {
    int n = 0;
    int m = 0;
    int U = 0;
    std::vector<flowshop::Job> jobs;
    std::vector<int> ui;
};

static void printInstanceSummary(const RandomInstance& inst) {
    std::cout << "\n=== RANDOM INSTANCE ===\n";
    std::cout << "n = " << inst.n << ", m = " << inst.m << ", U = " << inst.U << "\n";
    std::cout << "Jobs (id, p, w): ";
    if (inst.jobs.empty()) {
        std::cout << "(none)\n";
    } else {
        for (size_t i = 0; i < inst.jobs.size(); ++i) {
            const auto& j = inst.jobs[i];
            std::cout << "J" << (j.id + 1) << "(" << j.p << "," << j.w << ")";
            if (i + 1 < inst.jobs.size()) std::cout << "  ";
        }
        std::cout << "\n";
    }

    std::cout << "Outsourcing costs ui: ";
    if (inst.ui.empty()) {
        std::cout << "(none)\n";
    } else {
        for (size_t i = 0; i < inst.ui.size(); ++i) {
            std::cout << "J" << (inst.jobs[i].id + 1) << "=" << inst.ui[i];
            if (i + 1 < inst.ui.size()) std::cout << ", ";
        }
        std::cout << "\n";
    }
}

static RandomInstance generateRandomInstance(std::mt19937& rng) {
    // Keep n small so that the naive 2^n enumeration is still reasonable.
    // Also keep U modest so DP doesn't explode in states.
    std::uniform_int_distribution<int> distN(20, 25);
    std::uniform_int_distribution<int> distM(4, 8);
    std::uniform_int_distribution<int> distP(1, 20);
    std::uniform_int_distribution<int> distW(1, 10);
    std::uniform_int_distribution<int> distUi(10, 60);

    RandomInstance inst;
    inst.n = distN(rng);
    inst.m = distM(rng);

    inst.jobs.reserve(inst.n);
    inst.ui.reserve(inst.n);

    for (int i = 0; i < inst.n; ++i) {
        flowshop::Job j;
        j.id = i;
        j.p = distP(rng);
        j.w = distW(rng);
        inst.jobs.push_back(j);
        inst.ui.push_back(distUi(rng));
    }

    const int sumUi = static_cast<int>(std::accumulate(inst.ui.begin(), inst.ui.end(), 0LL));
    const int maxU = std::min(250, sumUi);
    const int minU = std::min(maxU, std::max(0, maxU / 4));
    if (maxU == 0) {
        inst.U = 0;
    } else {
        std::uniform_int_distribution<int> distU(minU, maxU);
        inst.U = distU(rng);
    }

    return inst;
}

template <typename Func>
static long long measureMicroseconds(Func&& func) {
    const auto start = std::chrono::steady_clock::now();
    func();
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

struct BenchmarkResult {
    flowshop_ext::NaiveResult naive;
    flowshop_ext::NaiveResult dp;
    long long naiveUs = 0;
    long long dpUs = 0;
};

static bool validateSameObjective(const flowshop_ext::NaiveResult& a,
                                  const flowshop_ext::NaiveResult& b) {
    return a.objective == b.objective;
}

static BenchmarkResult runAndBenchmark(const RandomInstance& inst) {
    BenchmarkResult out;

    out.naiveUs = measureMicroseconds([&]() {
        out.naive = flowshop_ext::solveNaiveDetailed(inst.jobs, inst.ui, inst.m, inst.U);
    });

    out.dpUs = measureMicroseconds([&]() {
        out.dp = flowshop_ext::solveDP(inst.jobs, inst.ui, inst.m, inst.U);
    });

    if (!validateSameObjective(out.naive, out.dp)) {
        std::cout << "\n[ERROR] Objective mismatch!\n";
        std::cout << "Naive objective = " << out.naive.objective << "\n";
        std::cout << "DP objective    = " << out.dp.objective << "\n";
        throw std::runtime_error("Naive and DP objectives do not match");
    }

    return out;
}

static void printBenchmarkSummary(const BenchmarkResult& r, int U) {
    const double naiveMs = r.naiveUs / 1000.0;
    const double dpMs = r.dpUs / 1000.0;

    std::cout << "\n=== BENCHMARK ===\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Naive time: " << naiveMs << " ms (" << r.naiveUs << " us)\n";
    std::cout << "DP time:    " << dpMs << " ms (" << r.dpUs << " us)\n";

    if (r.dpUs > 0) {
        const double speedup = static_cast<double>(r.naiveUs) / static_cast<double>(r.dpUs);
        std::cout << "Speedup:   " << speedup << "x faster (Naive/DP)\n";
    }

    std::cout << "\nCorrectness: objectives match (" << r.dp.objective << ")\n";

    // Keep the same detailed result output format you already had
    printNaiveResult(r.naive, U);
    printDPResult(r.dp, U);
}

static void runRandomDemoOnce() {
    std::random_device rd;
    std::mt19937 rng(rd() ^ static_cast<unsigned int>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()));

    RandomInstance inst = generateRandomInstance(rng);
    printInstanceSummary(inst);

    std::cout << "\nStarting Naive and DP comparison..." << std::endl;
    BenchmarkResult bench = runAndBenchmark(inst);
    printBenchmarkSummary(bench, inst.U);
}

int main() {
    try {
        runRandomDemoOnce();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "\nFatal error: " << ex.what() << std::endl;
        return 1;
    }
}