// Renderer.h
// Thin wrapper around SDL2 / SDL_ttf drawing primitives used by the app.
//
// Holds three pre-loaded font sizes plus a CPU icon texture, and exposes
// helpers for drawing text, filled rectangles, and outlined rectangles.
// All drawing routines go through an SDL_Renderer* the App owns; this
// class does not own the renderer itself.

#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <string>

class Renderer {
public:
    // Load fonts and the CPU icon. Returns false on any failure; check
    // SDL_GetError() / TTF_GetError() / IMG_GetError() to diagnose.
    bool init(SDL_Renderer* sdl);

    // Free fonts and textures. Safe to call multiple times.
    void shutdown();

    // Fill a solid color rectangle.
    void fillRect(const SDL_Rect& r, SDL_Color c);

    // Draw a rectangle outline (1px) in the given color.
    void drawRect(const SDL_Rect& r, SDL_Color c);

    // Draw a single line.
    void drawLine(int x1, int y1, int x2, int y2, SDL_Color c);

    // Render a UTF-8 string at (x, y) using one of the loaded fonts.
    // size selects a font: 0 = title, 1 = body, 2 = small / mono.
    // Returns the rendered text width in pixels (useful for layout) and
    // writes the rendered height to *out_h if non-null.
    int drawText(const std::string& text, int x, int y, SDL_Color c,
                 int size, int* out_h = nullptr);

    // Measure text without drawing.
    void measureText(const std::string& text, int size, int* w, int* h);

    // Blit the CPU icon at (x, y) scaled to (w, h). Falls back to a
    // procedurally-drawn icon if the texture failed to load.
    void drawCpuIcon(int x, int y, int w, int h);

private:
    SDL_Renderer* sdl_ = nullptr;
    TTF_Font* fonts_[3] = {nullptr, nullptr, nullptr};
    SDL_Texture* cpu_icon_ = nullptr;
};
