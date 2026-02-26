#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits>
#include <numeric>
#include <unordered_set>
#include <stdexcept>
#include "FlowShopWSPTMCI.cpp" // שימוש בקופסה השחורה הקיימת

namespace flowshop_ext {

struct NaiveResult {
    long long objective = 0;
    std::vector<flowshop::Job> inhouseOrder;
    std::vector<flowshop::Job> outsourced;
    long long outsourcingCost = 0;
};

struct DPState {
    long long objective = std::numeric_limits<long long>::max();
    std::vector<flowshop::Job> inhouseJobs;
};

NaiveResult solveNaiveDetailed(const std::vector<flowshop::Job>& allJobs,
                              const std::vector<int>& outsourcingCosts,
                              int m, int U);

NaiveResult solveDP(const std::vector<flowshop::Job>& allJobs,
                    const std::vector<int>& outsourcingCosts,
                    int m, int U);

// פונקציית מעטפת שמריצה את ה-MCI ומחזירה רק את הערך המספרי
// מקבלת את הווקטור של המטלות ואת מספר המכונות m
long long getObjectiveOnly(const std::vector<flowshop::Job>& jobs, int m) {
    if (jobs.empty()) return 0;
    
    // שליחה למימוש הקיים בקופסה השחורה
    // אנחנו משתמשים ב-solveWSPT_MCI ישירות כדי להימנע מהדפסות
    // dbgOut מועבר כ-ostream ריק (null)
    std::ostream nullOut(nullptr);
    flowshop::Solution sol = flowshop::solveWSPT_MCI(jobs, m, false, nullOut);
    
    return sol.objective;
}

static flowshop::Solution getSolutionOnly(const std::vector<flowshop::Job>& jobs, int m) {
    if (jobs.empty()) return flowshop::Solution{};

    std::ostream nullOut(nullptr);
    return flowshop::solveWSPT_MCI(jobs, m, false, nullOut);
}

// --- DP algorithm (Minimization knapsack variant) ---
// dp[i][c] = best (minimum) objective using first i jobs with outsourcing budget <= c.
// Each job is either kept in-house (added to the set evaluated by the black-box)
// or outsourced (spending u_i budget and not appearing in the black-box set).
NaiveResult solveDP(const std::vector<flowshop::Job>& allJobs,
                    const std::vector<int>& outsourcingCosts,
                    int m, int U) {

    const int n = static_cast<int>(allJobs.size());
    if (n != static_cast<int>(outsourcingCosts.size())) {
        throw std::invalid_argument("outsourcingCosts size must match allJobs size");
    }
    if (U < 0) {
        throw std::invalid_argument("U must be non-negative");
    }

    const long long INF = std::numeric_limits<long long>::max() / 4;

    for (int i = 0; i < n; ++i) {
        if (outsourcingCosts[i] < 0) {
            throw std::invalid_argument("outsourcingCosts must be non-negative");
        }
    }

    // Build DP table
    std::vector<std::vector<DPState>> dp(n + 1, std::vector<DPState>(U + 1));

    // Base: with 0 jobs, objective is 0 for any allowed budget.
    for (int c = 0; c <= U; ++c) {
        dp[0][c].objective = 0;
        dp[0][c].inhouseJobs.clear();
    }

    for (int i = 1; i <= n; ++i) {
        const flowshop::Job& job = allJobs[i - 1];
        const int u_i = outsourcingCosts[i - 1];

        for (int c = 0; c <= U; ++c) {
            DPState best;
            best.objective = INF;

            // Option 1: Keep in-house
            if (dp[i - 1][c].objective != INF) {
                std::vector<flowshop::Job> keepList = dp[i - 1][c].inhouseJobs;
                keepList.push_back(job);
                const long long keepObj = getObjectiveOnly(keepList, m);

                if (keepObj < best.objective) {
                    best.objective = keepObj;
                    best.inhouseJobs = std::move(keepList);
                }
            }

            // Option 2: Outsource (if budget allows)
            if (u_i <= c && dp[i - 1][c - u_i].objective != INF) {
                const long long outObj = dp[i - 1][c - u_i].objective;
                if (outObj < best.objective) {
                    best.objective = outObj;
                    best.inhouseJobs = dp[i - 1][c - u_i].inhouseJobs;
                }
            }

            dp[i][c] = std::move(best);
        }
    }

    // dp[n][U] already represents best objective with outsourcing budget <= U
    const DPState& bestState = dp[n][U];

    NaiveResult result;
    result.objective = bestState.objective == INF ? 0 : bestState.objective;

    // Final in-house sequence/order comes from the black-box solution
    if (!bestState.inhouseJobs.empty()) {
        flowshop::Solution sol = getSolutionOnly(bestState.inhouseJobs, m);
        result.objective = sol.objective;
        result.inhouseOrder = std::move(sol.sequence);
    }

    // Build outsourced list + outsourcingCost
    // (outsourced jobs are those NOT in bestState.inhouseJobs)
    std::unordered_set<int> inhouseIds;
    inhouseIds.reserve(bestState.inhouseJobs.size());
    for (const auto& j : bestState.inhouseJobs) {
        inhouseIds.insert(j.id);
    }

    long long outsourceCost = 0;
    result.outsourced.clear();
    result.outsourced.reserve(n);
    for (int idx = 0; idx < n; ++idx) {
        const auto& j = allJobs[idx];
        if (inhouseIds.find(j.id) == inhouseIds.end()) {
            result.outsourced.push_back(j);
            outsourceCost += outsourcingCosts[idx];
        }
    }

    result.outsourcingCost = outsourceCost;
    return result;
}

// --- אלגוריתם נאיבי (Brute Force) ---
// עובר על כל 2^n האפשרויות ומחזיר את ה-Objective המינימלי
long long solveNaive(const std::vector<flowshop::Job>& allJobs, 
                   const std::vector<int>& outsourcingCosts, 
                   int m, int U) {
    return solveNaiveDetailed(allJobs, outsourcingCosts, m, U).objective;
}

NaiveResult solveNaiveDetailed(const std::vector<flowshop::Job>& allJobs,
                              const std::vector<int>& outsourcingCosts,
                              int m, int U) {

    const int n = static_cast<int>(allJobs.size());
    if (n != static_cast<int>(outsourcingCosts.size())) {
        throw std::invalid_argument("outsourcingCosts size must match allJobs size");
    }
    if (n >= 63) {
        throw std::invalid_argument("solveNaiveDetailed supports up to 62 jobs (bitmask brute force)");
    }

    NaiveResult best;
    best.objective = std::numeric_limits<long long>::max();
    best.outsourcingCost = 0;

    const unsigned long long totalMasks = 1ULL << n;
    for (unsigned long long mask = 0; mask < totalMasks; ++mask) {
        std::vector<flowshop::Job> currentA;
        std::vector<flowshop::Job> currentOut;
        long long currentOutsourceCost = 0;

        currentA.reserve(n);
        currentOut.reserve(n);

        for (int j = 0; j < n; ++j) {
            if ((mask >> j) & 1ULL) {
                currentA.push_back(allJobs[j]);
            } else {
                currentOut.push_back(allJobs[j]);
                currentOutsourceCost += outsourcingCosts[j];
            }
        }

        if (currentOutsourceCost > U) continue;

        long long obj = 0;
        std::vector<flowshop::Job> order;
        if (!currentA.empty()) {
            flowshop::Solution sol = getSolutionOnly(currentA, m);
            obj = sol.objective;
            order = std::move(sol.sequence);
        }

        if (obj < best.objective) {
            best.objective = obj;
            best.inhouseOrder = std::move(order);
            best.outsourced = std::move(currentOut);
            best.outsourcingCost = currentOutsourceCost;
        }
    }

    if (best.objective == std::numeric_limits<long long>::max()) {
        // No feasible solution (shouldn't happen unless U < 0)
        best.objective = 0;
        best.inhouseOrder.clear();
        best.outsourced = allJobs;
        best.outsourcingCost = std::accumulate(outsourcingCosts.begin(), outsourcingCosts.end(), 0LL);
    }

    return best;
}

} // namespace flowshop_ext