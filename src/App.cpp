// App.cpp
// Main application loop and panel layout for the CPU Core Scheduling
// Visualizer. All on-screen panels are positioned from constants near the
// top of each draw method so tweaking the layout is local.

#include "App.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <cstdio>
#include <string>

#include "Types.h"

namespace {

// Shared color palette. Kept in an anonymous namespace so it doesn't leak.
constexpr SDL_Color kBg          = { 24,  26,  38, 255};
constexpr SDL_Color kPanel       = { 38,  42,  58, 255};
constexpr SDL_Color kPanelBorder = { 70,  78, 100, 255};
constexpr SDL_Color kText        = {235, 240, 250, 255};
constexpr SDL_Color kTextDim     = {170, 180, 200, 255};
constexpr SDL_Color kAccent      = {120, 180, 255, 255};
constexpr SDL_Color kRule        = { 60,  66,  86, 255};

} // namespace

bool App::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    if (TTF_Init() != 0) {
        std::fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        return false;
    }
    // We only need PNG support; requesting it explicitly produces a clearer
    // error message if libpng / SDL_image is misconfigured.
    int img_flags = IMG_INIT_PNG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        std::fprintf(stderr, "IMG_Init(PNG) failed: %s\n", IMG_GetError());
        // Continue anyway; the icon is non-essential.
    }

    window_ = SDL_CreateWindow(
        "CPU Core Scheduling Visualizer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H,
        SDL_WINDOW_SHOWN);
    if (!window_) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    sdl_ = SDL_CreateRenderer(
        window_, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!sdl_) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetRenderDrawBlendMode(sdl_, SDL_BLENDMODE_BLEND);

    if (!renderer_.init(sdl_)) {
        return false;
    }

    // Prime the simulation with a single tick so the timeline isn't empty
    // the moment the window opens.
    scheduler_.pushEvent("Simulator started");
    scheduler_.tick();
    last_tick_ms_ = SDL_GetTicks();
    return true;
}

void App::shutdown() {
    renderer_.shutdown();
    if (sdl_)    { SDL_DestroyRenderer(sdl_); sdl_ = nullptr; }
    if (window_) { SDL_DestroyWindow(window_); window_ = nullptr; }
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

void App::run() {
    while (running_) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            handleEvent(e);
        }
        update(SDL_GetTicks());
        render();
        // VSync controls pacing; this small delay caps the loop if vsync
        // is unavailable so we don't burn CPU.
        SDL_Delay(8);
    }
}

void App::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_QUIT) {
        running_ = false;
        return;
    }
    if (e.type != SDL_KEYDOWN) return;

    switch (e.key.keysym.sym) {
        case SDLK_ESCAPE:
        case SDLK_q:
            running_ = false;
            break;
        case SDLK_1:
            scheduler_.setAlgorithm(SchedAlgo::ROUND_ROBIN);
            break;
        case SDLK_2:
            scheduler_.setAlgorithm(SchedAlgo::PRIORITY);
            break;
        case SDLK_SPACE:
            paused_ = !paused_;
            scheduler_.pushEvent(paused_ ? "Paused" : "Resumed");
            break;
        case SDLK_r:
            scheduler_.reset();
            log_scroll_ = 0;
            break;
        case SDLK_UP:
            log_scroll_ = std::min(log_scroll_ + 1,
                                   static_cast<int>(scheduler_.events().size()));
            break;
        case SDLK_DOWN:
            log_scroll_ = std::max(log_scroll_ - 1, 0);
            break;
        case SDLK_PAGEUP:
            log_scroll_ = std::min(log_scroll_ + 5,
                                   static_cast<int>(scheduler_.events().size()));
            break;
        case SDLK_PAGEDOWN:
            log_scroll_ = std::max(log_scroll_ - 5, 0);
            break;
        default: break;
    }
}

void App::update(Uint32 now_ms) {
    if (paused_) {
        // Anchor last_tick_ms_ to "now" while paused so that a long pause
        // doesn't fire a burst of ticks when we resume.
        last_tick_ms_ = now_ms;
        return;
    }
    if (now_ms - last_tick_ms_ >= TICK_MS) {
        scheduler_.tick();
        last_tick_ms_ += TICK_MS;
        // If we fell badly behind (e.g. window minimized), don't try to
        // catch up by tick-spamming.
        if (now_ms - last_tick_ms_ > TICK_MS * 5) {
            last_tick_ms_ = now_ms;
        }
    }
}

void App::render() {
    SDL_SetRenderDrawColor(sdl_, kBg.r, kBg.g, kBg.b, kBg.a);
    SDL_RenderClear(sdl_);

    drawTitle();
    drawStats();
    drawLegend();
    drawTimelines();
    drawEventLog();
    drawControlsHint();

    SDL_RenderPresent(sdl_);
}

void App::drawTitle() {
    // Title bar with the CPU icon on the left and a horizontal rule below.
    const int icon_x = 18, icon_y = 14, icon_sz = 36;
    renderer_.drawCpuIcon(icon_x, icon_y, icon_sz, icon_sz);

    SDL_Color title_color = kText;
    renderer_.drawText("CPU Core Scheduling Visualizer",
                       icon_x + icon_sz + 14, icon_y + 2, title_color, 0);

    // Subtitle to the right of the title in a dimmer tone.
    int title_w = 0;
    renderer_.measureText("CPU Core Scheduling Visualizer", 0, &title_w, nullptr);
    renderer_.drawText("Operating Systems final project",
                       icon_x + icon_sz + 14 + title_w + 18, icon_y + 11,
                       kTextDim, 1);

    renderer_.drawLine(0, 62, WINDOW_W, 62, kRule);
}

void App::drawStats() {
    // Stats strip just below the title bar.
    const int y = 72;
    const int h = 36;
    SDL_Rect bg{12, y, WINDOW_W - 24, h};
    renderer_.fillRect(bg, kPanel);
    renderer_.drawRect(bg, kPanelBorder);

    char util[64];
    std::snprintf(util, sizeof(util), "CPU Utilization: %5.1f%%",
                  scheduler_.utilization() * 100.0);

    char active[64];
    std::snprintf(active, sizeof(active), "Active Processes: %d",
                  scheduler_.activeProcessCount());

    char algo[96];
    std::snprintf(algo, sizeof(algo), "Algorithm: %s",
                  scheduler_.algorithmName());

    int text_y = y + 10;
    int cx = 24;
    cx += renderer_.drawText(util, cx, text_y, kText, 1) + 32;
    cx += renderer_.drawText(active, cx, text_y, kText, 1) + 32;
    cx += renderer_.drawText(algo, cx, text_y, kAccent, 1) + 32;

    // Pause indicator anchored to the right side of the stats strip.
    if (paused_) {
        SDL_Color pause_col{255, 180, 120, 255};
        int pw = 0; renderer_.measureText("[PAUSED]", 1, &pw, nullptr);
        renderer_.drawText("[PAUSED]", WINDOW_W - 24 - pw, text_y,
                           pause_col, 1);
    }
}

void App::drawLegend() {
    // Process color key strip beneath the stats.
    const int y = 118;
    const int h = 30;
    SDL_Rect bg{12, y, WINDOW_W - 24, h};
    renderer_.fillRect(bg, kPanel);
    renderer_.drawRect(bg, kPanelBorder);

    int x = 24;
    int text_y = y + 8;
    renderer_.drawText("Legend:", x, text_y, kTextDim, 1);
    x += 70;

    const auto& procs = scheduler_.processes();
    for (size_t i = 0; i < procs.size(); ++i) {
        SDL_Rect sw{x, y + 8, 16, 14};
        renderer_.fillRect(sw, procs[i].color);
        renderer_.drawRect(sw, kPanelBorder);
        x += 22;
        int tw = renderer_.drawText(procs[i].name, x, text_y, kText, 1);
        x += tw + 28;
    }
}

void App::drawTimelines() {
    // Four horizontal core rows. Each block represents one scheduling slot.
    const int panel_x = 12;
    const int panel_y = 158;
    const int panel_w = WINDOW_W - 24;
    const int panel_h = 220;
    SDL_Rect bg{panel_x, panel_y, panel_w, panel_h};
    renderer_.fillRect(bg, kPanel);
    renderer_.drawRect(bg, kPanelBorder);

    const int label_w   = 80;   // column reserved for "Core N" labels
    const int row_pad_y = 12;
    const int row_h = (panel_h - row_pad_y * 2) / Scheduler::NUM_CORES;
    const int track_x = panel_x + label_w;
    const int track_w = panel_w - label_w - 16;

    const auto& procs = scheduler_.processes();
    const int slot_count = Scheduler::MAX_HISTORY;
    const float slot_w = static_cast<float>(track_w) / slot_count;

    for (int c = 0; c < Scheduler::NUM_CORES; ++c) {
        int row_y = panel_y + row_pad_y + c * row_h;

        // Label on the left.
        char label[16];
        std::snprintf(label, sizeof(label), "Core %d", c);
        renderer_.drawText(label, panel_x + 14, row_y + row_h / 2 - 10,
                           kText, 1);

        // Empty track background.
        SDL_Rect track{track_x, row_y + 4, track_w, row_h - 12};
        renderer_.fillRect(track, SDL_Color{30, 32, 46, 255});
        renderer_.drawRect(track, SDL_Color{55, 60, 80, 255});

        // Draw each historical scheduling slot. We anchor to the right so
        // new blocks slide in from the right edge.
        const auto& history = scheduler_.core(c);
        int n = static_cast<int>(history.size());
        for (int i = 0; i < n; ++i) {
            const Block& b = history[i];
            int slot_index_from_right = (n - 1) - i;
            float right_edge = track_x + track_w - slot_index_from_right * slot_w;
            float left_edge  = right_edge - slot_w + 1.0f;
            SDL_Rect r{
                static_cast<int>(left_edge),
                row_y + 5,
                std::max(1, static_cast<int>(slot_w - 1.0f)),
                row_h - 14
            };
            SDL_Color col = procs[static_cast<int>(b.pid)].color;
            renderer_.fillRect(r, col);
        }

        // Vertical "now" cursor at the right edge of the track.
        renderer_.drawLine(track_x + track_w + 1, row_y + 4,
                           track_x + track_w + 1, row_y + row_h - 8,
                           kAccent);
    }

    // Axis caption below the timelines.
    renderer_.drawText("<- older                                            newer ->",
                       track_x, panel_y + panel_h - 18, kTextDim, 2);
}

void App::drawEventLog() {
    // Bottom panel: scrollable event log.
    const int panel_x = 12;
    const int panel_y = 388;
    const int panel_w = WINDOW_W - 24;
    const int panel_h = 290;
    SDL_Rect bg{panel_x, panel_y, panel_w, panel_h};
    renderer_.fillRect(bg, kPanel);
    renderer_.drawRect(bg, kPanelBorder);

    // Title bar inside the panel.
    renderer_.drawText("Event Log", panel_x + 12, panel_y + 8, kAccent, 1);

    char scroll_info[64];
    std::snprintf(scroll_info, sizeof(scroll_info),
                  "scroll: %d  (up/down to scroll, PgUp/PgDn faster)",
                  log_scroll_);
    int sw = 0; renderer_.measureText(scroll_info, 2, &sw, nullptr);
    renderer_.drawText(scroll_info, panel_x + panel_w - sw - 12,
                       panel_y + 12, kTextDim, 2);

    renderer_.drawLine(panel_x + 8, panel_y + 32,
                       panel_x + panel_w - 8, panel_y + 32, kRule);

    // Lay out lines from the bottom up so the newest message sits at the
    // bottom of the panel and older messages stack upward. log_scroll_ shifts
    // the window backward into history.
    const int line_h = 16;
    const int top_y  = panel_y + 40;
    const int bot_y  = panel_y + panel_h - 12;
    const int max_lines = (bot_y - top_y) / line_h;

    const auto& evs = scheduler_.events();
    int total = static_cast<int>(evs.size());
    int end_index = total - 1 - log_scroll_; // index of newest visible event
    if (end_index < 0) end_index = -1;       // nothing visible

    for (int i = 0; i < max_lines; ++i) {
        int idx = end_index - i;
        if (idx < 0) break;
        int y = bot_y - line_h - i * line_h;
        // Slight zebra striping for readability.
        if ((i & 1) == 0) {
            SDL_Rect stripe{panel_x + 8, y - 1, panel_w - 16, line_h};
            renderer_.fillRect(stripe, SDL_Color{32, 35, 50, 255});
        }
        char numbered[256];
        std::snprintf(numbered, sizeof(numbered), "[%04d]  %s",
                      idx, evs[idx].c_str());
        renderer_.drawText(numbered, panel_x + 14, y, kText, 2);
    }

    if (total == 0) {
        renderer_.drawText("(no events yet)", panel_x + 14, top_y + 4,
                           kTextDim, 2);
    }
}

void App::drawControlsHint() {
    // Single-line help text pinned to the very bottom of the window.
    const char* hint =
        "1: Round Robin   2: Priority   Space: Pause   R: Reset   "
        "Up/Down: Scroll log   Esc/Q: Quit";
    int y = WINDOW_H - 18;
    int w = 0; renderer_.measureText(hint, 2, &w, nullptr);
    renderer_.drawText(hint, (WINDOW_W - w) / 2, y, kTextDim, 2);
}
