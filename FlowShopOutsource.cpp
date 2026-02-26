#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include "FlowShopWSPTMCI.cpp" // שימוש בקופסה השחורה הקיימת

namespace flowshop_ext {

struct NaiveResult {
    long long objective = 0;
    std::vector<flowshop::Job> inhouseOrder;
    std::vector<flowshop::Job> outsourced;
    long long outsourcingCost = 0;
};

NaiveResult solveNaiveDetailed(const std::vector<flowshop::Job>& allJobs,
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