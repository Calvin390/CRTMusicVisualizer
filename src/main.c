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

#define CONFIG_PATH "config/layout.json"
#define MAX_VISUALIZER_BARS 256

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

/*
 * Configuration defaults
 */

static void configSetDefaults(AppConfig *config)
{
    if (config == NULL) {
        return;
    }

    *config = (AppConfig) {
        .logicalWidth = 320,
        .logicalHeight = 240,

        .physicalWidthInches = 4.0f,
        .physicalHeightInches = 3.0f,
        .previewPixelsPerInch = 96.0f,

        /*
         * Keep paths lowercase because Linux paths are case-sensitive.
         * Replace these placeholder filenames with real assets later.
         */
        .fontPath = "assets/fonts/TBD.ttf",
        .fontSize = 15.0f,

        .albumArtPath = "assets/images/TBD.png",

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

/*
 * JSON helpers
 */

static char *readEntireFile(const char *path)
{
    if (path == NULL) {
        return NULL;
    }

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

    const size_t bytesRead = fread(
        buffer,
        1U,
        (size_t)length,
        file
    );

    fclose(file);

    if (bytesRead != (size_t)length) {
        free(buffer);
        return NULL;
    }


    buffer[(size_t)length] = '\0';

    return buffer;
}

static void jsonReadInt(
    const cJSON *object,
    const char *name,
    int *destination)
{
    if (object == NULL ||
        name == NULL ||
        destination == NULL) {
        return;
    }

    const cJSON *item =
        cJSON_GetObjectItemCaseSensitive(object, name);

    if (cJSON_IsNumber(item)) {
        *destination = item->valueint;
    }
}

static void jsonReadFloat(
    const cJSON *object,
    const char *name,
    float *destination)
{
    if (object == NULL ||
        name == NULL ||
        destination == NULL) {
        return;
    }

    const cJSON *item =
        cJSON_GetObjectItemCaseSensitive(object, name);

    if (cJSON_IsNumber(item)) {
        *destination = (float)item->valuedouble;
    }
}

static void jsonReadString(
    const cJSON *object,
    const char *name,
    char *destination,
    size_t destinationSize)
{
    if (object == NULL ||
        name == NULL ||
        destination == NULL ||
        destinationSize == 0U) {
        return;
    }

    const cJSON *item =
        cJSON_GetObjectItemCaseSensitive(object, name);

    if (!cJSON_IsString(item) ||
        item->valuestring == NULL) {
        return;
    }

    SDL_strlcpy(
        destination,
        item->valuestring,
        destinationSize
    );
}

static void jsonReadRect(
    const cJSON *parent,
    const char *name,
    LayoutRectangle *rect)
{
    if (parent == NULL ||
        name == NULL ||
        rect == NULL) {
        return;
    }

    const cJSON *object =
        cJSON_GetObjectItemCaseSensitive(parent, name);

    if (!cJSON_IsObject(object)) {
        return;
    }

    jsonReadFloat(object, "x", &rect->x);
    jsonReadFloat(object, "y", &rect->y);
    jsonReadFloat(object, "width", &rect->width);
    jsonReadFloat(object, "height", &rect->height);
}


static bool configValidate(AppConfig *config)
{
    if (config == NULL) {
        return false;
    }


    if (config->logicalWidth <= 0 ||
        config->logicalHeight <= 0) {
        SDL_Log(
            "Invalid logical resolution; restoring configuration defaults"
        );

        configSetDefaults(config);
        return false;
    }

    if (config->physicalWidthInches <= 0.0f) {
        SDL_Log(
            "Invalid physicalWidthInches; restoring default"
        );

        config->physicalWidthInches = 4.0f;
    }

    if (config->physicalHeightInches <= 0.0f) {
        SDL_Log(
            "Invalid physicalHeightInches; restoring default"
        );

        config->physicalHeightInches = 3.0f;
    }

    if (config->previewPixelsPerInch <= 0.0f) {
        SDL_Log(
            "Invalid previewPixelsPerInch; restoring default"
        );

        config->previewPixelsPerInch = 96.0f;
    }

    if (config->fontSize <= 0.0f) {
        SDL_Log(
            "Invalid fontSize; restoring default"
        );

        config->fontSize = 15.0f;
    }


    if (config->albumArt.width < 0.0f) {
        config->albumArt.width = 0.0f;
    }

    if (config->albumArt.height < 0.0f) {
        config->albumArt.height = 0.0f;
    }

    if (config->visualizer.width < 0.0f) {
        config->visualizer.width = 0.0f;
    }

    if (config->visualizer.height < 0.0f) {
        config->visualizer.height = 0.0f;
    }

    if (config->visualizerBars < 1) {
        config->visualizerBars = 1;
    }

    if (config->visualizerBars > MAX_VISUALIZER_BARS) {
        config->visualizerBars = MAX_VISUALIZER_BARS;
    }

    if (config->safeLeft < 0.0f) {
        config->safeLeft = 0.0f;
    }

    if (config->safeRight < 0.0f) {
        config->safeRight = 0.0f;
    }

    if (config->safeTop < 0.0f) {
        config->safeTop = 0.0f;
    }

    if (config->safeBottom < 0.0f) {
        config->safeBottom = 0.0f;
    }

    return true;
}


static bool configLoad(
    AppConfig *config,
    const char *path)
{
    if (config == NULL || path == NULL) {
        return false;
    }

    configSetDefaults(config);

    char *text = readEntireFile(path);

    if (text == NULL) {
        SDL_Log(
            "Could not read %s; using defaults",
            path
        );

        return false;
    }

    cJSON *root = cJSON_Parse(text);

    free(text);

    if (root == NULL) {
        SDL_Log(
            "Could not parse %s; using defaults",
            path
        );

        return false;
    }


    if (!cJSON_IsObject(root)) {
        SDL_Log(
            "Root of %s is not a JSON object; using defaults",
            path
        );

        cJSON_Delete(root);
        return false;
    }

    jsonReadInt(
        root,
        "logicalWidth",
        &config->logicalWidth
    );

    jsonReadInt(
        root,
        "logicalHeight",
        &config->logicalHeight
    );

    jsonReadFloat(
        root,
        "physicalWidthInches",
        &config->physicalWidthInches
    );

    jsonReadFloat(
        root,
        "physicalHeightInches",
        &config->physicalHeightInches
    );

    jsonReadFloat(
        root,
        "previewPixelsPerInch",
        &config->previewPixelsPerInch
    );

    jsonReadString(
        root,
        "fontPath",
        config->fontPath,
        sizeof(config->fontPath)
    );

    jsonReadFloat(
        root,
        "fontSize",
        &config->fontSize
    );

    jsonReadString(
        root,
        "albumArtPath",
        config->albumArtPath,
        sizeof(config->albumArtPath)
    );

    jsonReadRect(
        root,
        "albumArt",
        &config->albumArt
    );

    jsonReadRect(
        root,
        "visualizer",
        &config->visualizer
    );

    const cJSON *title =
        cJSON_GetObjectItemCaseSensitive(root, "title");

    if (cJSON_IsObject(title)) {
        jsonReadFloat(
            title,
            "x",
            &config->titleX
        );

        jsonReadFloat(
            title,
            "y",
            &config->titleY
        );
    }

    const cJSON *artist =
        cJSON_GetObjectItemCaseSensitive(root, "artist");

    if (cJSON_IsObject(artist)) {
        jsonReadFloat(
            artist,
            "x",
            &config->artistX
        );

        jsonReadFloat(
            artist,
            "y",
            &config->artistY
        );
    }

    const cJSON *visualizer =
        cJSON_GetObjectItemCaseSensitive(root, "visualizer");

    if (cJSON_IsObject(visualizer)) {
        jsonReadInt(
            visualizer,
            "bars",
            &config->visualizerBars
        );
    }

    const cJSON *safeArea =
        cJSON_GetObjectItemCaseSensitive(root, "safeArea");

    if (cJSON_IsObject(safeArea)) {
        jsonReadFloat(
            safeArea,
            "left",
            &config->safeLeft
        );

        jsonReadFloat(
            safeArea,
            "right",
            &config->safeRight
        );

        jsonReadFloat(
            safeArea,
            "top",
            &config->safeTop
        );

        jsonReadFloat(
            safeArea,
            "bottom",
            &config->safeBottom
        );
    }

    /*
     * Release the memory allocated by cJSON_Parse.
     */
    cJSON_Delete(root);

    return configValidate(config);
}

/*
 * Asset helpers
 */

static void textTextureDestroy(TextTexture *textTexture) {

    if (textTexture == NULL) {
        return;
    }

    if (textTexture->texture != NULL) {
        SDL_DestroyTexture(textTexture->texture);
    }

    *textTexture = (TextTexture) {0};
}

static SDL_Texture *imageTextureLoad(SDL_Renderer *renderer, const char *path) {

    if (renderer == NULL || path == NULL || path[0] == '\0') {
        return NULL;
    }

    SDL_Texture *texture = IMG_LoadTexture(renderer, path);

    if (texture == NULL) {
        SDL_Log("Could not load image '%s' : %s", path, SDL_GetError());
        return NULL;
    }
    return texture;
}

static bool textTextureBuild(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Color color, TextTexture *output) {
    if (renderer == NULL || font == NULL || text == NULL || output == NULL) {
        return false;
    }

    *output = (TextTexture) {0};

    SDL_Surface *surface = TTF_RenderText_Blended(font, text, 0, color);

    if (surface == NULL) {
        SDL_Log("Could not render '%s': %s", text, SDL_GetError());

        return false;
    }

    output->width = (float)surface->w;
    output->height = (float)surface->h;

    output->texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_DestroySurface(surface);

    if (output->texture == NULL) {
        SDL_Log("Could not create texture for text '%s': %s", text, SDL_GetError());

        output->width = 0.0f;
        output->height = 0.0f;

        return false;
    }

    return true;
}

static void assetsDestroy(App *app) {
    if (app == NULL) {
        return;
    }

    textTextureDestroy(&app->title);
    textTextureDestroy(&app->artist);

    if (app->albumArt != NULL) {

        SDL_DestroyTexture(app->albumArt);
        app->albumArt = NULL;
    }

    if (app->font != NULL) {
        TTF_CloseFont(app->font);
        app->font = NULL;
    }
}

static bool assetsLoad(App *app, const char *titleText, const char *artistText) {

    if (app == NULL || app->renderer == NULL || titleText == NULL || artistText == NULL) {
        return false;
    }

    SDL_Texture *newAlbumArt = NULL;
    TTF_Font *newFont = NULL;

    TextTexture newTitle = {0};
    TextTexture newArtist = {0};

    const SDL_Color textColor ={
        .r = 235,
        .g = 235,
        .b = 220,
        .a = 255
    };

    newAlbumArt = imageTextureLoad(app->renderer, app->config.albumArtPath);

    if (newAlbumArt == NULL) {
        goto loadFailure;
    }

    newFont = TTF_OpenFont(app->config.fontPath, app->config.fontSize);

    if (newFont == NULL) {
        SDL_Log("Could not open font '%s': %s", app->config.fontPath, SDL_GetError());
        goto loadFailure;
    }

    if (!textTextureBuild(app->renderer, newFont, titleText, textColor, &newTitle)) {
        goto loadFailure;
    }

    if (!textTextureBuild(app->renderer, newFont, artistText, textColor, &newArtist)) {
        goto loadFailure;
    }

    assetsDestroy(app);

    app->albumArt = newAlbumArt;
    app->font = newFont;
    app->title = newTitle;
    app->artist = newArtist;

    return true;

    loadFailure:
        textTextureDestroy(&newTitle);
        textTextureDestroy(&newArtist);

        if (newAlbumArt != NULL) {
            SDL_DestroyTexture(newAlbumArt);
        }

        if (newFont != NULL) {
            TTF_CloseFont(newFont);
        }

        return false;

}

static bool assetsUpdateText(
    App *app,
    const char *titleText,
    const char *artistText)
{
    if (app == NULL ||
        app->renderer == NULL ||
        app->font == NULL ||
        titleText == NULL ||
        artistText == NULL) {
        return false;
        }

    const SDL_Color textColor = {
        .r = 235,
        .g = 235,
        .b = 220,
        .a = 255
    };

    TextTexture newTitle = {0};
    TextTexture newArtist = {0};

    if (!textTextureBuild(
            app->renderer,
            app->font,
            titleText,
            textColor,
            &newTitle)) {
        return false;
            }

    if (!textTextureBuild(
            app->renderer,
            app->font,
            artistText,
            textColor,
            &newArtist)) {
        textTextureDestroy(&newTitle);
        return false;
            }


    textTextureDestroy(&app->title);
    textTextureDestroy(&app->artist);

    app->title = newTitle;
    app->artist = newArtist;

    return true;
}

//Rendering Helpers

static bool renderTextureInRectangle(SDL_renderer *renderer, SDL_Texture *texture, const LayoutRectangle *rectangle) {
    if (renderer == NULL || texture == NULL || rectangle == NULL) {
        return false;
    }

    const SDL_FRect destination = {
        .x = rectangle->x,
        .y = rectangle->y,
        .w = rectangle->width,
        .h = rectangle->height
    };

    return SDL_RenderTexture(renderer, texture, NULL, &destination);
}

static bool renderTextTexture(SDL_Renderer *renderer, const TextTexture *textTexture, float x, float y) {
    if (renderer == NULL || textTexture == NULL || textTexture->texture == NULL) {
        return false;
    }

    const SDL_FRect destination = {
        .x = x,
        .y = y,
        .w = textTexture->width,
        .h = textTexture->height
    };

    return SDL_RenderTexture(renderer, textTexture->texture, NULL, &destination);
}

static bool renderVisualizer(App *app, float timeSeconds) {
    if (app == NULL || app->renderer == NULL) {
        return false;
    }

    const LayoutRectangle *area = &app->config.visualizer;

    const int barCount = app->config.visualizerBars;

    if (barCount <= 0 || area->width <= 0.0f || area->height <= 0.0f) {
        return false;
    }

    const float gap = 2.0f;

    const float totalGap = gap * (float)(barCount - 1);

    const float barWidth = (area->width - totalGap) / (float)barCount;

    if (barWidth <= 0.0f) {
        return false;
    }

    if (!SDL_SetRenderDrawColor(app->renderer, 255, 170, 60, 255)) {
        return false;
    }
    return true;
}

static bool renderSafeAreaGuides(App *app) {
    if (app == NULL || app->renderer == NULL) {
        return false;
    }

    const float width = (float)app->config.logicalWidth - app->config.safeLeft - app->config.safeRight;

    const float height = (float)app->config.logicalHeight - app->config.safeTop - app->config.safeBottom;

    if (width <= 0.0f || height <= 0.0f) {
        return false;
    }

    const SDL_FRect safeArea = {
        .x = app->config.safeLeft,
        .y = app->config.safeTop,
        .w = width,
        .h = height
    };

    if (!SDL_SetRenderDrawColor(app->renderer, 110, 110, 110, 255)) {
        return false;
    }

    return SDL_RenderRect(app->renderer, &safeArea);
}

static bool renderFrame(App *app) {
    if (app == NULL || app->renderer == NULL) {
        return false;
    }

    if (!SDL_SetRenderDrawColor(app->renderer, 5, 4, 3, 255)) {
        return false;
    }

    if (!SDL_RenderClear(app->renderer)) {
        return false;
    }

    if (app->albumArt != NULL) {
        if (!renderTextureInRectangle(app->renderer, app->albumArt, &app->config.albumArt)) {
            return false;
        }
    }

    if (app->title.texture != NULL) {
        if (!renderTextTexture(app->renderer, &app->title, app->config.titleX, app->config.titleY)) {
            return false;
        }
    }

    if (app->artist.texture != NULL) {
        if (!renderTextTexture(app->renderer, &app->artist, app->config.artistX, app->config.artistY)) {
            return false;
        }
    }

    const float timeSeconds = (float)SDL_GetTicks() / 1000.0f;

    if (!renderVisualizer(app, timeSeconds)) {
        return false;
    }

    if (app->showGuides) {
        if (!renderSafeAreaGuides(app)) {
            return false;
        }
    }

    SDL_RenderPresent(app->renderer);

    return true;
}
