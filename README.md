# Flow Shop Scheduling with Outsourcing (Naive vs DP)

This project solves a **Flow Shop Scheduling** problem with an **Outsourcing/Rejection** extension.
It compares a brute-force **Naive (2^n)** solver against a **Dynamic Programming (DP)** solver and verifies they return the same objective.

## Installation

### 1) Clone

```bash
git clone https://github.com/OmerCohen7530/FlowShop.git
cd FlowShop
```

### 2) Dependencies

- No external libraries.
- You only need a **C++17** compiler (see Requirements).

### 3) Build

**Linux / macOS (bash):**
```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o flowshop
```

**Windows (PowerShell, MinGW g++):**
```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o flowshop.exe
```

> If your folder path contains non-ASCII characters and linking fails, build the output to an ASCII path:
```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o C:\Temp\flowshop.exe
```

### 4) Run

**Linux / macOS:**
```bash
./flowshop
```

**Windows:**
```powershell
.\flowshop.exe
# or
C:\Temp\flowshop.exe
```

## Usage

When you run the program it will:
- Generate a random test instance: `n`, `m`, processing times `p_i`, weights `w_i`, outsourcing costs `u_i`, and budget `U`
- Run:
  - `solveNaiveDetailed(...)` (brute force)
  - `solveDP(...)` (dynamic programming)
- Verify **both objectives match**
- Print execution time for each method and the speedup

## Project Structure

- `main.cpp`
  - Program entry point
  - Generates random instances, runs both solvers, validates correctness, and prints benchmark results
- `FlowShopOutsource.cpp`
  - Outsourcing extension logic
  - Includes:
    - `solveNaiveDetailed(...)` (brute force)
    - `solveDP(...)` (minimization DP over outsourcing budget)
    - helper wrappers to call the black-box scheduler
- `FlowShopWSPTMCI.cpp`
  - “Black Box” scheduler: `flowshop::solveWSPT_MCI(...)`
  - Given an in-house job set, it returns the optimal in-house sequence and objective

**How files connect**
- `main.cpp` calls `solveNaiveDetailed(...)` and `solveDP(...)` from `FlowShopOutsource.cpp`.
- Both solvers evaluate an in-house job list by calling the black-box `flowshop::solveWSPT_MCI(...)` in `FlowShopWSPTMCI.cpp`.

## Build & Development

- Development build (fast compile, easier debugging):
```bash
g++ -std=c++17 -O0 -g -Wall -Wextra -pedantic main.cpp -o flowshop
```

- Production build (optimized):
```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o flowshop
```

## Requirements

- C++ compiler with **C++17** support
  - Windows: MinGW-w64 `g++` (tested)
  - Linux/macOS: `g++` or `clang++`
