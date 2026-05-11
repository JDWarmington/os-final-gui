// Scheduler.cpp
// Implementation of the simulated CPU scheduler.

#include "Scheduler.h"

#include <SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <set>

Scheduler::Scheduler()
    : rng_(static_cast<unsigned>(
          std::chrono::steady_clock::now().time_since_epoch().count())) {
    // Catalog of processes that may be scheduled. Colors are chosen to be
    // distinct on a dark background. Priority only matters for the
    // priority-scheduling policy.
    processes_ = {
        {"Process A", SDL_Color{ 92, 170, 255, 255}, 5}, // blue
        {"Process B", SDL_Color{120, 220, 130, 255}, 3}, // green
        {"Process C", SDL_Color{255, 170,  80, 255}, 2}, // orange
        {"Idle",      SDL_Color{ 90,  90, 110, 255}, 0}, // muted gray
    };

    for (int i = 0; i < NUM_CORES; ++i) {
        last_assigned_[i] = ProcessId::IDLE;
    }
}

void Scheduler::reset() {
    for (int i = 0; i < NUM_CORES; ++i) {
        cores_[i].clear();
        last_assigned_[i] = ProcessId::IDLE;
    }
    events_.clear();
    tick_count_ = 0;
    rr_offset_ = 0;
    pushEvent("Visualization reset");
}

void Scheduler::setAlgorithm(SchedAlgo algo) {
    if (algo == algorithm_) return;
    algorithm_ = algo;
    pushEvent(std::string("Switched to ") + algorithmName());
}

const char* Scheduler::algorithmName() const {
    switch (algorithm_) {
        case SchedAlgo::ROUND_ROBIN: return "Round Robin";
        case SchedAlgo::PRIORITY:    return "Priority Scheduling";
    }
    return "Unknown";
}

void Scheduler::tick() {
    ++tick_count_;
    switch (algorithm_) {
        case SchedAlgo::ROUND_ROBIN: tickRoundRobin(); break;
        case SchedAlgo::PRIORITY:    tickPriority();   break;
    }
}

void Scheduler::tickRoundRobin() {
    // Cycle through {A, B, C, IDLE} across cores and rotate the starting
    // offset every tick so each core sees every process over time. With
    // 4 cores and 4 entries this gives a clean repeating barber-pole.
    static constexpr std::array<ProcessId, 4> order = {
        ProcessId::A, ProcessId::B, ProcessId::C, ProcessId::IDLE,
    };
    for (int i = 0; i < NUM_CORES; ++i) {
        ProcessId pid = order[(i + rr_offset_) % order.size()];
        recordAssignment(i, pid);
    }
    rr_offset_ = (rr_offset_ + 1) % static_cast<int>(order.size());
}

void Scheduler::tickPriority() {
    // Probabilistic priority-weighted assignment. Each core independently
    // samples a process where higher-priority processes are more likely.
    // A small idle probability is preserved so the bars still show gaps.
    std::array<int, 4> weights = {
        processes_[0].priority * 10 + 5, // A
        processes_[1].priority * 10 + 5, // B
        processes_[2].priority * 10 + 5, // C
        12,                              // IDLE: small fixed weight
    };
    int total = weights[0] + weights[1] + weights[2] + weights[3];

    std::uniform_int_distribution<int> dist(0, total - 1);
    for (int i = 0; i < NUM_CORES; ++i) {
        int roll = dist(rng_);
        ProcessId pid = ProcessId::IDLE;
        int acc = 0;
        for (int j = 0; j < 4; ++j) {
            acc += weights[j];
            if (roll < acc) { pid = static_cast<ProcessId>(j); break; }
        }
        recordAssignment(i, pid);
    }
}

void Scheduler::recordAssignment(int core, ProcessId pid) {
    Block b{pid, SDL_GetTicks()};
    cores_[core].push_back(b);
    while (static_cast<int>(cores_[core].size()) > MAX_HISTORY) {
        cores_[core].pop_front();
    }

    // Log only transitions, not every repeat assignment, otherwise the log
    // would fill instantly. Idle transitions get their own phrasing.
    if (pid != last_assigned_[core]) {
        char buf[96];
        if (pid == ProcessId::IDLE) {
            std::snprintf(buf, sizeof(buf), "Core %d idle", core);
        } else {
            std::snprintf(buf, sizeof(buf), "%s assigned to Core %d",
                          processes_[static_cast<int>(pid)].name.c_str(),
                          core);
        }
        pushEvent(buf);
        last_assigned_[core] = pid;
    }
}

void Scheduler::pushEvent(const std::string& msg) {
    events_.push_back(msg);
    while (static_cast<int>(events_.size()) > MAX_EVENTS) {
        events_.pop_front();
    }
}

double Scheduler::utilization() const {
    // Computed over the most recent slot of each core. Anything that is
    // not IDLE counts as utilized.
    int total = 0, busy = 0;
    for (int i = 0; i < NUM_CORES; ++i) {
        // Average over the last few slots for stability.
        int look = std::min<int>(8, static_cast<int>(cores_[i].size()));
        for (int k = 0; k < look; ++k) {
            const Block& b = cores_[i][cores_[i].size() - 1 - k];
            ++total;
            if (b.pid != ProcessId::IDLE) ++busy;
        }
    }
    if (total == 0) return 0.0;
    return static_cast<double>(busy) / static_cast<double>(total);
}

int Scheduler::activeProcessCount() const {
    std::set<ProcessId> seen;
    for (int i = 0; i < NUM_CORES; ++i) {
        if (cores_[i].empty()) continue;
        ProcessId p = cores_[i].back().pid;
        if (p != ProcessId::IDLE) seen.insert(p);
    }
    return static_cast<int>(seen.size());
}
