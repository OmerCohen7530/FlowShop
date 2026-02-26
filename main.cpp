#include <iostream>
#include <vector>
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

int main() {
    using flowshop::Job;
    const int m = 2; // מספר המכונות
    const int U = 100; // תקציב מקסימלי ל-Outsourcing

    // הגדרת המטלות (Ji, Pi, Wi)
    std::vector<Job> jobs = {
        {0, 2, 4}, 
        {1, 4, 1},
        {2, 3, 2}
    };

    // עלויות ה-Outsourcing לכל מטלה (ui)
    std::vector<int> ui = {120, 30, 100};

    std::cout << "Starting Naive Algorithm..." << std::endl;
    flowshop_ext::NaiveResult naiveResult = flowshop_ext::solveNaiveDetailed(jobs, ui, m, U);

    printNaiveResult(naiveResult, U);

    std::cout << "\nStarting DP Algorithm..." << std::endl;
    flowshop_ext::NaiveResult dpResult = flowshop_ext::solveDP(jobs, ui, m, U);

    printDPResult(dpResult, U);

    return 0;
}