// main.cpp
// Entry point. Constructs the App, runs it, and tears it down.
//
// SDL2's main-replacement macro requires the (int, char**) signature on
// some platforms even when we don't use the arguments.

#include "App.h"

int main(int /*argc*/, char* /*argv*/[]) {
    App app;
    if (!app.init()) {
        app.shutdown();
        return 1;
    }
    app.run();
    app.shutdown();
    return 0;
}
