// App.h
// Top-level application: owns the SDL window/renderer, the simulated
// Scheduler, and the Renderer helper, and drives the main event/update/
// draw loop.

#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "Renderer.h"
#include "Scheduler.h"

class App {
public:
    // Window dimensions. Tuned to keep all panels visible without resizing.
    static constexpr int WINDOW_W = 1024;
    static constexpr int WINDOW_H = 720;

    // How often (ms) the simulation advances. Matches the assignment's
    // "every 500ms" requirement.
    static constexpr Uint32 TICK_MS = 500;

    bool init();
    void run();
    void shutdown();

private:
    // Process one queued SDL event.
    void handleEvent(const SDL_Event& e);

    // Possibly advance the simulation, depending on paused state and time.
    void update(Uint32 now_ms);

    // Draw a full frame.
    void render();

    // Draw helpers for individual panels.
    void drawTitle();
    void drawStats();
    void drawLegend();
    void drawTimelines();
    void drawEventLog();
    void drawControlsHint();

    SDL_Window* window_ = nullptr;
    SDL_Renderer* sdl_ = nullptr;

    Renderer renderer_;
    Scheduler scheduler_;

    Uint32 last_tick_ms_ = 0;
    bool paused_ = false;
    bool running_ = true;

    // Vertical scroll offset for the event log, in entries. 0 = newest
    // visible at the bottom; positive values reveal older entries.
    int log_scroll_ = 0;
};
