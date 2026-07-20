#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct {
    float x;
    float y;
    float width;
    float height;
} LayoutRectangle;

typedef struct {
    int logicalWidth;
    int logicalHeight;

    float physicalWidthInches;
    float physicalHeightInches;
    float previewPixelsPerInch;

    char fontPath[260];
    float fontSize;

    char albumArtPath[260];
}
