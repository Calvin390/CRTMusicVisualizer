#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "cJSON.h"

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

    LayoutRectangle albumArt;

    float titleX;
    float titleY;

    float artistX;
    float artistY;

    LayoutRectangle visualizer;
    int visualizerBars;

    float safeLeft;
    float safeRight;
    float safeTop;
    float safeBottom;
} AppConfig;

typedef struct {
    SDL_Texture *texture;
    float width;
    float height;
} TextTexture;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;

    SDL_Texture *albumArt;
    TTF_Font *font;

    TextTexture title;
    TextTexture artist;

    AppConfig config;

    bool running;
    bool showGuides;
    bool physicalSizeMode;
} App;

//Configuration Defaults

static void config_set_defaults(AppConfig *config) {
    *config = (AppConfig) {
        .logicalWidth = 320,
        .logicalHeight = 240,

        .physicalWidthInches = 4.0f,
        .physicalHeightInches = 3.0f,
        .previewPixelsPerInch = 96.0f,

        .fontPath = "Assets/Fonts/TBD.tff",
        .fontSize = 15.0f,

        .albumArtPath = "Assets/Images/TBD.png",

        .albumArt = {
        .x = 10.0f,
        .y = 10.0f,
        .width = 104.0f,
        .height = 104.0f
        },

        .titleX = 126.0f,
        .titleY = 14.0f,

        .artistX = 126.0f,
        .artistY = 39.0f,

        .visualizer = {
        .x = 10.0f,
        .y = 150.0f,
        .width = 300.0f,
        .height = 72.0f
        },

        .visualizerBars = 24,

        .safeLeft = 8.0f,
        .safeRight = 8.0f,
        .safeTop = 8.0f,
        .safeBottom = 8.0f
    };
}

//JSON Helpers
static char *readEntireFile(const char *path) {
    FILE *file = fopen(path, "rb");

    if (file == NULL) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    const long length = ftell(file);

    if (length < 0) {
        fclose(file);
        return NULL;
    }

    rewind(file);

    char *buffer = malloc((size_t)length + 1U);

    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    const size_t bytesRead = fread(buffer, 1U, (size_t)length, file);

    fclose(file);

    if (bytesRead != (size_t)length) {
        free(buffer);
        return NULL;
    }
    buffer[length] = '\0';
    return buffer;
}

static void jsonReadInt(const cJSON *object, const char *name, int *destination) {
    const cJSON *item = cJSON_GetObjectItem(object, name);

    if (cJSON_IsNumber(item)) {
        *destination = item->valueint;
    }
}

static void jsonReadFloat(const cJSON *object, const char *name, float *destination) {
    const cJSON *item = cJSON_GetObjectItem(object, name);

    if (cJSON_IsNumber(item)) {
        *destination = (float)item->valuedouble;
    }
}

static void jsonReadString(const cJSON *object, const char *name, char *destination, size_t destinationSize) {
    const cJSON *item = cJSON_GetObjectItem(object, name);

    if (!cJSON_IsString(item) || item->valuestring == NULL || destinationSize == 0U) {
        return;
    }
    SDL_strlcpy(destination, item->valuestring, destinationSize);
}

static void jsonReadRect(const cJSON *parent, const char *name, LayoutRectangle *rect) {
    const cJSON *object = cJSON_GetObjectItemCaseSensitive(parent, name);

    if (!cJSON_IsObject(object)) {
        return;
    }

    jsonReadFloat(object, "x", &rect->x);
    jsonReadFloat(object, "y", &rect->y);
    jsonReadFloat(object, "width", &rect->width);
    jsonReadFloat(object, "height", &rect->height);
}

static bool configLoad(AppConfig *config, const char *path) {
    config_set_defaults(config);

    char *text = readEntireFile(path);

    if (text == NULL) {
        SDL_Log("Could not read %s; using defaults", path);
        return false;
    }

    cJSON *root = cJSON_Parse(text);
    free(text);

    if (root == NULL) {
        SDL_Log("Could not parse %s; using defaults", path);
        return false;
    }

    jsonReadInt(root, "logicalWidth", &config->logicalWidth);

    jsonReadInt(root, "logicalHeight", &config->logicalHeight);

    jsonReadFloat(root, "physicalWidthInches", &config->physicalWidthInches);

    jsonReadFloat(root, "physicalHeightInches", &config->physicalHeightInches);

    jsonReadFloat(root, "previewPixelsPerInch", &config->previewPixelsPerInch);

    jsonReadString(root, "fontPath", config->fontPath, sizeof(config->fontPath));

    jsonReadFloat(root, "fontSize", &config->fontSize);

    jsonReadString(root, "albumArtPath", config->albumArtPath, sizeof(config->albumArtPath));

    jsonReadRect(root, "albumArt", &config->albumArt);
    jsonReadRect(root, "visualizer", &config->visualizer);

    const cJSON *title = cJSON_GetObjectItemCaseSensitive(root, "title");

    if (cJSON_IsObject(title)) {
        jsonReadFloat(title, "x", &config->titleX);
        jsonReadFloat(title, "y", &config->titleY);
    }

    const cJSON *artist = cJSON_GetObjectItemCaseSensitive(root, "artist");

    if (cJSON_IsObject(artist)) {
        jsonReadFloat(artist, "x", &config->artistX);
        jsonReadFloat(artist, "y", &config->artistY);
    }

}