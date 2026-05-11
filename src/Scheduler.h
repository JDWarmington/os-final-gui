// Scheduler.h
// Owns the simulated scheduling state for the visualizer.
//
// The Scheduler does NOT control the real operating system scheduler.
// It generates plausible-looking scheduling traces for four cores using
// either a round-robin or a priority-based policy and exposes the recent
// history plus some derived stats for the renderer to display.

#pragma once

#include <deque>
#include <random>
#include <string>
#include <vector>

#include "Types.h"

class Scheduler {
public:
    // Number of simulated CPU cores shown on screen.
    static constexpr int NUM_CORES = 4;

    // How many historical scheduling slots we retain per core.
    // At a 500ms tick, 60 slots is 30s of visible history per core.
    static constexpr int MAX_HISTORY = 60;

    // How many event-log lines we keep before dropping the oldest.
    static constexpr int MAX_EVENTS = 200;

    Scheduler();

    // Clear all timeline history and event log; keep the current algorithm.
    void reset();

    // Advance the simulation by one scheduling tick. This assigns a
    // (possibly idle) process to every core based on the current algorithm
    // and may push messages into the event log.
    void tick();

    // Switch policies. Pushes an event entry so the user can see the
    // transition in the log panel.
    void setAlgorithm(SchedAlgo algo);

    SchedAlgo algorithm() const { return algorithm_; }
    const char* algorithmName() const;

    // Access to the per-core scheduling history (oldest at front).
    const std::deque<Block>& core(int i) const { return cores_[i]; }

    // The process catalog (names + colors + priorities).
    const std::vector<ProcessInfo>& processes() const { return processes_; }

    // Fraction of the most recent slots across all cores that are non-idle.
    // Returned as a value in [0.0, 1.0].
    double utilization() const;

    // Number of distinct non-idle processes scheduled in the most recent slot.
    int activeProcessCount() const;

    // Append a free-form message to the event log (used for user-driven
    // events like "Switched to Priority Scheduling").
    void pushEvent(const std::string& msg);

    const std::deque<std::string>& events() const { return events_; }

    int tickCount() const { return tick_count_; }

private:
    void tickRoundRobin();
    void tickPriority();
    void recordAssignment(int core, ProcessId pid);

    std::deque<Block> cores_[NUM_CORES];
    std::vector<ProcessInfo> processes_;
    std::deque<std::string> events_;

    SchedAlgo algorithm_ = SchedAlgo::ROUND_ROBIN;
    int tick_count_ = 0;
    int rr_offset_ = 0;            // rotates round-robin assignment each tick
    ProcessId last_assigned_[NUM_CORES]; // for change detection in the log
    std::mt19937 rng_;
};
