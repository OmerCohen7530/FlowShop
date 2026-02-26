#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace flowshop {

struct Job {
    int id;               // 0-based job id
    long long p;          // processing time (same on every machine)
    long long w;          // weight
};

struct Solution {
    std::vector<Job> sequence;     // final schedule
    long long objective = 0;       // sum w_j * C_j (on last machine)
};

// ---------- Utility: compute C_j (last machine) using the closed-form formula ----------
// For a given sequence (jobs indexed by position r = 1..n):
// C_r = sum_{k=1..r} p_k + (m-1) * max{p_1..p_r}
//
// Then objective = sum_{r=1..n} w_r * C_r
static long long computeObjectiveClosedForm(const std::vector<Job>& seq, int m) {
    if (m <= 0) throw std::invalid_argument("m must be positive");
    long long sumP = 0;
    long long maxP = 0;
    long long obj  = 0;

    for (const auto& job : seq) {
        sumP += job.p;
        if (job.p > maxP) maxP = job.p;
        long long C = sumP + 1LL * (m - 1) * maxP;
        obj += job.w * C;
    }
    return obj;
}

// Optional verification: compute objective by classic DP table C[pos][machine]
// C[i][k] = max(C[i-1][k], C[i][k-1]) + p(job_i)
// with p(job_i) same for every machine k.
// This should match the closed-form objective for proportional case.
static long long computeObjectiveDP(const std::vector<Job>& seq, int m) {
    const int n = static_cast<int>(seq.size());
    std::vector<std::vector<long long>> C(n, std::vector<long long>(m, 0));

    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < m; ++k) {
            long long up   = (i > 0) ? C[i - 1][k] : 0LL;
            long long left = (k > 0) ? C[i][k - 1] : 0LL;
            C[i][k] = std::max(up, left) + seq[i].p;
        }
    }

    long long obj = 0;
    for (int i = 0; i < n; ++i) {
        long long C_last = C[i][m - 1];
        obj += seq[i].w * C_last;
    }
    return obj;
}

// ---------- Printing helpers (Excel-like table) ----------
static void printFinalTable(const std::vector<Job>& seq, int m, std::ostream& out) {
    out << "\n=== Final schedule details (closed-form) ===\n";
    out << "m = " << m << "\n\n";

    out << std::left
        << std::setw(6)  << "pos"
        << std::setw(6)  << "job"
        << std::setw(8)  << "p"
        << std::setw(8)  << "w"
        << std::setw(12) << "sumP"
        << std::setw(12) << "maxP"
        << std::setw(14) << "C_last"
        << std::setw(14) << "w*C"
        << std::setw(14) << "cumObj"
        << "\n";

    out << std::string(6+6+8+8+12+12+14+14+14, '-') << "\n";

    long long sumP = 0, maxP = 0, cumObj = 0;
    for (int i = 0; i < (int)seq.size(); ++i) {
        const auto& job = seq[i];
        sumP += job.p;
        if (job.p > maxP) maxP = job.p;
        long long C_last = sumP + 1LL * (m - 1) * maxP;
        long long wc     = job.w * C_last;
        cumObj += wc;

        out << std::left
            << std::setw(6)  << (i + 1)
            << std::setw(6)  << ("J" + std::to_string(job.id + 1))
            << std::setw(8)  << job.p
            << std::setw(8)  << job.w
            << std::setw(12) << sumP
            << std::setw(12) << maxP
            << std::setw(14) << C_last
            << std::setw(14) << wc
            << std::setw(14) << cumObj
            << "\n";
    }
}

// ---------- Core algorithm: WSPT-MCI ----------
// Step 0: re-index jobs by non-increasing w/p (WSPT order).
// Step 1: S1 = [job1]
// Step 2: for k=2..n: insert job k into S_{k-1} at position minimizing increase in objective.
// Tie-break: if multiple positions give the same best objective, insert LATEST (rightmost).
static Solution solveWSPT_MCI(std::vector<Job> jobs, int m, bool verifyDP, std::ostream& dbgOut) {
    if (m <= 0) throw std::invalid_argument("m must be positive");
    if (jobs.empty()) throw std::invalid_argument("jobs list is empty");

    // Sort by decreasing w/p. Use cross-multiplication to avoid floating errors:
    // w1/p1 >= w2/p2  <=>  w1*p2 >= w2*p1
    std::stable_sort(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        __int128 left  = (__int128)a.w * (__int128)b.p;
        __int128 right = (__int128)b.w * (__int128)a.p;
        if (left != right) return left > right;  // decreasing ratio
        // Optional tie-break: smaller p first (doesn't hurt, keeps stable behavior)
        return a.p < b.p;
    });

    std::vector<Job> S;
    S.push_back(jobs[0]);

    for (int k = 1; k < (int)jobs.size(); ++k) {
        const Job newJob = jobs[k];

        long long bestObj = std::numeric_limits<long long>::max();
        int bestPos = 0; // insertion position index in [0..S.size()]
        
        // Try all insertion positions
        for (int pos = 0; pos <= (int)S.size(); ++pos) {
            std::vector<Job> candidate = S;
            candidate.insert(candidate.begin() + pos, newJob);

            long long obj = computeObjectiveClosedForm(candidate, m);

            // Tie-break: choose the LATEST insertion (largest pos) if equal objective
            if (obj < bestObj || (obj == bestObj && pos > bestPos)) {
                bestObj = obj;
                bestPos = pos;
            }
        }

        S.insert(S.begin() + bestPos, newJob);

        // dbgOut << "Inserted " << ("J" + std::to_string(newJob.id + 1))
        //        << " at position " << (bestPos + 1)
        //        << " | Obj = " << bestObj << "\n";
    }

    Solution sol;
    sol.sequence = std::move(S);
    sol.objective = computeObjectiveClosedForm(sol.sequence, m);

    if (verifyDP) {
        long long objDP = computeObjectiveDP(sol.sequence, m);
        if (objDP != sol.objective) {
            throw std::runtime_error("Verification failed: DP objective != closed-form objective");
        }
    }

    return sol;
}

// Public runner that prints everything you typically need.
static Solution runAndPrint(std::vector<Job> jobs,
                            int m,
                            bool verifyDP = true,
                            bool printTable = true,
                            bool printDebugInsertions = true,
                            std::ostream& out = std::cout) {
    // if (printDebugInsertions) {
    //     out << "=== WSPT-MCI build log ===\n";
    // }
    Solution sol = solveWSPT_MCI(std::move(jobs), m, verifyDP,
                                 printDebugInsertions ? out : *(new std::ostream(nullptr)));

    // Print final order + objective
    out << "\n=== RESULT ===\n";
    out << "Final order: ";
    for (size_t i = 0; i < sol.sequence.size(); ++i) {
        out << "J" << (sol.sequence[i].id + 1);
        if (i + 1 < sol.sequence.size()) out << " -> ";
    }
    out << "\nObjective (sum w_j * C_j on last machine) = " << sol.objective << "\n";

    // if (printTable) {
    //     printFinalTable(sol.sequence, m, out);
    // }

    // if (verifyDP) {
    //     out << "\n[OK] Verified objective using DP table equals closed-form.\n";
    // }

    return sol;
}

} // namespace flowshop
