// Renderer.cpp
// SDL2 drawing helpers. Loads fonts from a list of candidate paths so the
// project works whether the user dropped a TTF into assets/fonts or relies
// on the system font cache.

#include "Renderer.h"

#include <cstdio>
#include <vector>

namespace {

// Candidate font paths searched in order. The first one that loads wins.
// The bundled location is preferred so the project is self-contained when
// the user supplies their own TTF.
const std::vector<const char*> kFontCandidates = {
    "assets/fonts/font.ttf",
    "assets/fonts/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/TTF/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/Library/Fonts/Arial.ttf",
    "C:/Windows/Fonts/arial.ttf",
};

TTF_Font* openFirstAvailableFont(int pt) {
    for (const char* path : kFontCandidates) {
        TTF_Font* f = TTF_OpenFont(path, pt);
        if (f) return f;
    }
    return nullptr;
}

} // namespace

bool Renderer::init(SDL_Renderer* sdl) {
    sdl_ = sdl;

    fonts_[0] = openFirstAvailableFont(28); // title
    fonts_[1] = openFirstAvailableFont(16); // body
    fonts_[2] = openFirstAvailableFont(13); // small / log
    if (!fonts_[0] || !fonts_[1] || !fonts_[2]) {
        std::fprintf(stderr,
                     "Renderer: failed to load any TTF font.\n"
                     "Drop a .ttf into assets/fonts/font.ttf or install\n"
                     "a system font (e.g. fonts-dejavu on Debian).\n");
        return false;
    }

    // The CPU icon is optional; if missing we draw a fallback shape so the
    // app still runs end-to-end.
    SDL_Surface* surf = IMG_Load("assets/images/cpu.png");
    if (!surf) {
        // Try a BMP fallback (SDL_image also handles BMP through IMG_Load).
        surf = IMG_Load("assets/images/cpu.bmp");
    }
    if (surf) {
        cpu_icon_ = SDL_CreateTextureFromSurface(sdl_, surf);
        SDL_FreeSurface(surf);
    } else {
        std::fprintf(stderr,
                     "Renderer: assets/images/cpu.png not found; drawing\n"
                     "a procedural icon instead.\n");
    }

    return true;
}

void Renderer::shutdown() {
    for (int i = 0; i < 3; ++i) {
        if (fonts_[i]) { TTF_CloseFont(fonts_[i]); fonts_[i] = nullptr; }
    }
    if (cpu_icon_) { SDL_DestroyTexture(cpu_icon_); cpu_icon_ = nullptr; }
    sdl_ = nullptr;
}

void Renderer::fillRect(const SDL_Rect& r, SDL_Color c) {
    SDL_SetRenderDrawColor(sdl_, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(sdl_, &r);
}

void Renderer::drawRect(const SDL_Rect& r, SDL_Color c) {
    SDL_SetRenderDrawColor(sdl_, c.r, c.g, c.b, c.a);
    SDL_RenderDrawRect(sdl_, &r);
}

void Renderer::drawLine(int x1, int y1, int x2, int y2, SDL_Color c) {
    SDL_SetRenderDrawColor(sdl_, c.r, c.g, c.b, c.a);
    SDL_RenderDrawLine(sdl_, x1, y1, x2, y2);
}

int Renderer::drawText(const std::string& text, int x, int y, SDL_Color c,
                       int size, int* out_h) {
    if (text.empty()) { if (out_h) *out_h = 0; return 0; }
    TTF_Font* f = fonts_[size < 0 ? 0 : (size > 2 ? 2 : size)];
    SDL_Surface* surf = TTF_RenderUTF8_Blended(f, text.c_str(), c);
    if (!surf) { if (out_h) *out_h = 0; return 0; }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(sdl_, surf);
    int w = surf->w, h = surf->h;
    SDL_FreeSurface(surf);
    if (!tex) { if (out_h) *out_h = 0; return 0; }
    SDL_Rect dst{x, y, w, h};
    SDL_RenderCopy(sdl_, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
    if (out_h) *out_h = h;
    return w;
}

void Renderer::measureText(const std::string& text, int size, int* w, int* h) {
    TTF_Font* f = fonts_[size < 0 ? 0 : (size > 2 ? 2 : size)];
    int tw = 0, th = 0;
    TTF_SizeUTF8(f, text.c_str(), &tw, &th);
    if (w) *w = tw;
    if (h) *h = th;
}

void Renderer::drawCpuIcon(int x, int y, int w, int h) {
    if (cpu_icon_) {
        SDL_Rect dst{x, y, w, h};
        SDL_RenderCopy(sdl_, cpu_icon_, nullptr, &dst);
        return;
    }
    // Fallback: procedurally draw a small "chip" so the layout doesn't
    // shift when the texture is missing.
    SDL_Color outline{180, 200, 230, 255};
    SDL_Color body{60, 100, 160, 255};
    SDL_Color pad{200, 210, 230, 255};
    fillRect({x + w / 6, y + h / 6, w - w / 3, h - h / 3}, body);
    drawRect({x + w / 6, y + h / 6, w - w / 3, h - h / 3}, outline);
    int pin_len = w / 8;
    for (int i = 0; i < 3; ++i) {
        int off = h / 4 + i * (h / 4);
        fillRect({x, y + off, pin_len, 2}, pad);
        fillRect({x + w - pin_len, y + off, pin_len, 2}, pad);
        fillRect({x + off, y, 2, pin_len}, pad);
        fillRect({x + off, y + h - pin_len, 2, pin_len}, pad);
    }
}
