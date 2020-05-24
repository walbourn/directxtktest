// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This test ensures that all public headers fully include all their dependancies, as well compile cleanly at maximum warning level

extern void audiotest();
extern void bufferhelperstest();
extern void commonstatestest();
extern void ddstextureloadertest();
extern void directxhelperstest();
extern void effectstest();
extern void gamepadtest();
extern void geometricprimitivetest();
extern void graphicsmemorytest();
extern void keyboardtest();
extern void modeltest();
extern void mousetest();
extern void postprocesstest();
extern void primitivebatchtest();
extern void screengrabtest();
extern void simplemathtest();
extern void spritebatchtest();
extern void spritefonttest();
extern void vertextypestest();
extern void wictextureloadertest();

int main()
{
    audiotest();
    bufferhelperstest();
    commonstatestest();
    ddstextureloadertest();
    directxhelperstest();
    effectstest();
    gamepadtest();
    geometricprimitivetest();
    graphicsmemorytest();
    keyboardtest();
    modeltest();
    mousetest();
    postprocesstest();
    primitivebatchtest();
    screengrabtest();
    simplemathtest();
    spritebatchtest();
    spritefonttest();
    vertextypestest();
    wictextureloadertest();

    return 0;
}
