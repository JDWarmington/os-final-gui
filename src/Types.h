// Types.h
// Shared types used across the CPU Core Scheduling Visualizer.
// Keeps small data structures and enums in one place so other modules
// don't pull in the heavier Scheduler / Renderer headers just to use them.

#pragma once

#include <SDL.h>
#include <string>

// Identifiers for the simulated processes. IDLE represents a core that is
// not running any user process during a given scheduling slot.
enum class ProcessId {
    A = 0,
    B = 1,
    C = 2,
    IDLE = 3,
    COUNT = 4
};

// The two scheduling policies the user can toggle between at runtime.
enum class SchedAlgo {
    ROUND_ROBIN = 0,
    PRIORITY = 1
};

// A single scheduling slot recorded for a core. We store the process that
// ran during this slot and the SDL tick at which the slot was created so
// the timeline can fade or anchor blocks by time if we want to later.
struct Block {
    ProcessId pid;
    Uint32 timestamp_ms;
};

// Display metadata for each process: its name, the color used to draw it
// on the timeline / legend, and a relative priority weight used by the
// priority scheduling simulation.
struct ProcessInfo {
    std::string name;
    SDL_Color color;
    int priority; // higher = preferred under priority scheduling
};
