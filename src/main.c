#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "tinyexpr.h"

/*
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define SCREEN_ORIGIN_X (SCREEN_WIDTH / 2)
#define SCREEN_ORIGIN_Y (SCREEN_HEIGHT / 2)
*/

#define WINDOW_TITLE "Function Plotter" 
#define FONT_PATH "assets/Roboto-Bold.ttf" 

int SCREEN_WIDTH = 800;
int SCREEN_HEIGHT = 800;
int SCREEN_ORIGIN_X = 400;
int SCREEN_ORIGIN_Y = 400;

typedef struct {
    float x;
    float y;
    float zoom;
} Camera;

typedef struct {
    char buffer[128];
    SDL_Color color;
    SDL_Surface* surface;
    SDL_Texture* texture;
    SDL_Rect rect;
} Text;

void pollInput(SDL_Event* event, SDL_Window* window, bool* running,  bool* zoomChanged, bool* mouseClicked, Camera* cam, char* buffer);
void freeTextSurfaceAndTexture(SDL_Renderer* renderer, TTF_Font* font, Text* text);
int worldToScreenX(Camera* cam, float wx);
int worldToScreenY(Camera* cam, float wy);
double screenToWorldX(Camera* cam, int sx);
double screenToWorldY(Camera* cam, int sy);
void cleanupSDL(SDL_Window* window, SDL_Renderer* renderer, Text* zoomText, Text* exprText, Text* mouseText);
int initSDL(SDL_Window** window, SDL_Renderer** renderer);
void drawAxes(SDL_Renderer* renderer, Camera* camera);
void renderIndicators(SDL_Renderer* renderer, Camera* camera, TTF_Font* font);
int plotFunction(SDL_Renderer* renderer, Camera* camera, char* strExpr, SDL_Color* functionColor);
void removeTrailingZeroes(char* str);

int main(int argc, char** argv) {

    SDL_Color functionColor;

    if (argc < 2) {
        printf("Usage: ./main.exe \"<expression>\" \"<optional-color>\" \n");
        printf("Colors: \"BLUE\" | \"RED\" | \"GREEN\" | \"BLACK\" | \"YELLOW\" \n");  
        printf("Press enter to continue...\n"); 
        getchar();
        return EXIT_SUCCESS;
    } 

    printf("Expression to be evaluated: %s\n", argv[1]);

    if (argc > 2) {
        printf("Function color: %s\n", argv[2]);
        if ((strcmp(argv[2], "BLUE")) == 0) functionColor = (SDL_Color){0, 0, 150, 255};
        else if ((strcmp(argv[2], "RED")) == 0) functionColor = (SDL_Color){235, 0, 0, 255};
        else if ((strcmp(argv[2], "BLACK")) == 0) functionColor = (SDL_Color){0, 0, 0, 255};
        else if ((strcmp(argv[2], "GREEN")) == 0) functionColor = (SDL_Color){0, 150, 0, 255};
        else if ((strcmp(argv[2], "YELLOW")) == 0) functionColor = (SDL_Color){200, 200, 0, 255};
    } else {
        functionColor = (SDL_Color){235, 0, 0, 255};
    }

    SDL_Window* window;
    SDL_Renderer* renderer;

    if (initSDL(&window, &renderer) == EXIT_FAILURE) {
        cleanupSDL(window, renderer, NULL, NULL, NULL);
        fprintf(stderr, "SDL Initialization failed, exiting...\n");
        return EXIT_FAILURE;
    } 

    TTF_Font* font = TTF_OpenFont(FONT_PATH, 24);

    bool running = true;
    bool zoomChanged = false;
    bool mouseClicked = false;
    SDL_Event event;

    Camera camera;
    camera.x = 0.0f;
    camera.y = 0.0f;
    camera.zoom = 40.0f;

    Text zoomText, exprText, mouseText;

    snprintf(zoomText.buffer, sizeof(zoomText.buffer), "Zoom: %.2f", camera.zoom);
    zoomText.color = (SDL_Color){0, 0, 0, 255};
    zoomText.surface = TTF_RenderText_Solid(font, zoomText.buffer, zoomText.color);
    zoomText.texture = SDL_CreateTextureFromSurface(renderer, zoomText.surface);
    zoomText.rect = (SDL_Rect){25, 25, zoomText.surface->w, zoomText.surface->h};

    snprintf(exprText.buffer, sizeof(zoomText.buffer), "f(x) = %s", argv[1]);
    exprText.color = (SDL_Color){0, 0, 0, 255};
    exprText.surface = TTF_RenderText_Solid(font, exprText.buffer, exprText.color);
    exprText.texture = SDL_CreateTextureFromSurface(renderer, exprText.surface);
    exprText.rect = (SDL_Rect){25, 75, exprText.surface->w, exprText.surface->h};

    mouseText.color = (SDL_Color){0, 0, 0, 255};
    mouseText.surface = TTF_RenderText_Solid(font, mouseText.buffer, mouseText.color);
    mouseText.texture = SDL_CreateTextureFromSurface(renderer, mouseText.surface);

    printf("Starting function plotter...\n");

    int smx, smy;

    while (running) {
        SDL_GetMouseState(&smx, &smy);

        mouseText.rect = (SDL_Rect){smx, smy, mouseText.surface->w, mouseText.surface->h};

        pollInput(&event, window, &running, &zoomChanged, &mouseClicked, &camera, zoomText.buffer);

        if (zoomChanged) {
            freeTextSurfaceAndTexture(renderer, font, &zoomText);
            zoomChanged = false;
        }

        if (mouseClicked) {
            double x = screenToWorldX(&camera, smx);
            te_variable vars[] = {{"x", &x}};
            te_expr* expression = te_compile(argv[1], vars, 1, 0);

            char strX[32];
            char strY[32];
            snprintf(strX, sizeof(strX), "%.2f", x);
            snprintf(strY, sizeof(strY), "%.2f", te_eval(expression));
            removeTrailingZeroes(strX);
            removeTrailingZeroes(strY);

            snprintf(mouseText.buffer, sizeof(mouseText.buffer), "(%s, %s)", strX, strY);

            freeTextSurfaceAndTexture(renderer, font, &mouseText);
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        drawAxes(renderer, &camera);

        if (plotFunction(renderer, &camera, argv[1], &functionColor) == EXIT_FAILURE) {
            fprintf(stderr, "Received exit failure while plotting function, exiting...\n");
            return EXIT_FAILURE;
        };

        SDL_RenderCopy(renderer, zoomText.texture, NULL, &zoomText.rect);
        SDL_RenderCopy(renderer, exprText.texture, NULL, &exprText.rect);
        if (mouseClicked) SDL_RenderCopy(renderer, mouseText.texture, NULL, &mouseText.rect);

        renderIndicators(renderer, &camera, font);

        SDL_RenderPresent(renderer);
    }   

    printf("Exiting function plotter...\n");

    cleanupSDL(window, renderer, &zoomText, &exprText, &mouseText);

    return EXIT_SUCCESS;
}

void removeTrailingZeroes(char* str) {
    char* p = str + strlen(str) - 1; // Last character
    while (p > str && *p == '0') { // Loop through the text, from the last character, checking if the pointer points to a zero character
        *p-- = '\0'; // Set the 0 character to a null terminator
    }
    if (*p == '.') { // After removing trailing zeros, remove the decimal point
        *p = '\0';
    }
}

void renderIndicators(SDL_Renderer* renderer, Camera* camera, TTF_Font* font) {
    float worldLeft = camera->x - SCREEN_ORIGIN_X / camera->zoom;
    float worldRight = camera->x + SCREEN_ORIGIN_X / camera->zoom;
    float worldTop = camera->y - SCREEN_ORIGIN_Y / camera->zoom;
    float worldBottom = camera->y + SCREEN_ORIGIN_Y / camera->zoom;

    float gridStep = 100.0f / camera->zoom; 

    float pow10 = powf(10, floorf(log10f(gridStep)));
    float normalized = gridStep / pow10;

    if (normalized < 2.0f) gridStep = 1.0f * pow10;
    else if (normalized < 5.0f) gridStep = 2.0f * pow10;
    else gridStep = 5.0f * pow10;

    float startX = floorf(worldLeft / gridStep) * gridStep; 
    float startY = floorf(worldTop / gridStep) * gridStep;

    int textW, textH, sampleW, sampleH;

    float pixelStep = gridStep * camera->zoom; // Pixels in between grid axes

    char sample[32];
    snprintf(sample, sizeof(sample), "%.1f", worldRight);

    TTF_SizeText(font, sample, &sampleW, &sampleH);

    int XlabelInterval = 1;
    if (pixelStep < sampleW * 1.5f) { // Needed space is 150% normal width of text, compare with pixel space. If our needed space exceeds the distance, establish an interval to draw the labels.
        XlabelInterval = (int)ceilf((sampleW * 1.5f) / pixelStep);
    }

    // e.g:
    // needed size = 90px
    // pixel step = 30px
    // interval = 90 / 30 = 3
    // index == 0 > draw || index == 3 > draw || index == 6 > draw || ...
    // pixel steps adds up like 30 + 30 + 30 in between labels, meaning we'll have enough size to draw after 3 pixel steps.
    // draw one label every 3 grid lines

    int index = 0;
    for (float x = startX; x <= worldRight; x += gridStep, index++) {
        if (index % XlabelInterval != 0) continue; 
        Text indicatorText;
        snprintf(indicatorText.buffer, sizeof(indicatorText.buffer), "%.1f", x);
        removeTrailingZeroes(indicatorText.buffer);

        indicatorText.color = (SDL_Color){0, 0, 0, 255};
        indicatorText.surface = TTF_RenderText_Solid(font, indicatorText.buffer, indicatorText.color);
        indicatorText.texture = SDL_CreateTextureFromSurface(renderer, indicatorText.surface);
        textW = indicatorText.surface->w;
        textH = indicatorText.surface->h;
        indicatorText.rect = (SDL_Rect){worldToScreenX(camera, x) - textW / 2, worldToScreenY(camera, 0), textW, textH};
        SDL_RenderCopy(renderer, indicatorText.texture, NULL, &indicatorText.rect);
        SDL_FreeSurface(indicatorText.surface);
        SDL_DestroyTexture(indicatorText.texture);
    }

    int YlabelInterval = 1;
    if (pixelStep < sampleH * 1.5f) { // Needed space is 150% normal width of text, compare with pixel space. If our needed space exceeds the distance, establish an interval to draw the labels.
        YlabelInterval = (int)ceilf((sampleH * 1.5f) / pixelStep);
    }

    index = 0;
    for (float y = startY; y <= worldBottom; y += gridStep, index++) {
        if (index % YlabelInterval != 0) continue; 
        Text indicatorText;
        snprintf(indicatorText.buffer, sizeof(indicatorText.buffer), "%.1f", y * -1);
        removeTrailingZeroes(indicatorText.buffer);

        indicatorText.color = (SDL_Color){0, 0, 0, 255};
        indicatorText.surface = TTF_RenderText_Solid(font, indicatorText.buffer, indicatorText.color);
        indicatorText.texture = SDL_CreateTextureFromSurface(renderer, indicatorText.surface);
        textW = indicatorText.surface->w;
        textH = indicatorText.surface->h;
        indicatorText.rect = (SDL_Rect){worldToScreenX(camera, 0), worldToScreenY(camera, y) - textH / 2, textW, textH};
        SDL_RenderCopy(renderer, indicatorText.texture, NULL, &indicatorText.rect);
        SDL_FreeSurface(indicatorText.surface);
        SDL_DestroyTexture(indicatorText.texture);
    }
}

void freeTextSurfaceAndTexture(SDL_Renderer* renderer, TTF_Font* font, Text* text) {
    SDL_FreeSurface(text->surface);
    SDL_DestroyTexture(text->texture);
    text->surface = TTF_RenderText_Solid(font, text->buffer, text->color);
    text->texture = SDL_CreateTextureFromSurface(renderer, text->surface);
}

void drawAxes(SDL_Renderer* renderer, Camera* camera) {
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 128);
    float worldLeft = camera->x - SCREEN_ORIGIN_X / camera->zoom;
    float worldRight = camera->x + SCREEN_ORIGIN_X / camera->zoom;
    float worldTop = camera->y - SCREEN_ORIGIN_Y / camera->zoom;
    float worldBottom = camera->y + SCREEN_ORIGIN_Y / camera->zoom;

    float gridStep = 100.0f / camera->zoom; 

    float pow10 = powf(10, floorf(log10f(gridStep)));
    float normalized = gridStep / pow10;

    if (normalized < 2.0f) gridStep = 1.0f * pow10;
    else if (normalized < 5.0f) gridStep = 2.0f * pow10;
    else gridStep = 5.0f * pow10;

    float startX = floorf(worldLeft / gridStep) * gridStep; 
    float startY = floorf(worldTop / gridStep) * gridStep;

    for (float x = startX; x <= worldRight; x += gridStep) {
        int sx1, sy1, sx2, sy2;
        sx1 = worldToScreenX(camera, x);
        sy1 = worldToScreenY(camera, worldTop);
        sx2 = worldToScreenX(camera, x);
        sy2 = worldToScreenY(camera, worldBottom);
        SDL_RenderDrawLine(renderer, sx1, sy1, sx2, sy2);
    }

    for (float y = startY; y <= worldBottom; y += gridStep) {
        int sx1, sy1, sx2, sy2;
        sx1 = worldToScreenX(camera, worldLeft);
        sy1 = worldToScreenY(camera, y);
        sx2 = worldToScreenX(camera, worldRight);
        sy2 = worldToScreenY(camera, y);
        SDL_RenderDrawLine(renderer, sx1, sy1, sx2, sy2);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    int originX, originY;
    originX = worldToScreenX(camera, 0);
    originY = worldToScreenY(camera, 0);
    SDL_RenderDrawLine(renderer, 0, originY, SCREEN_WIDTH, originY);
    SDL_RenderDrawLine(renderer, originX, 0, originX, SCREEN_HEIGHT);
}

int plotFunction(SDL_Renderer* renderer, Camera* camera, char* strExpr, SDL_Color* functionColor) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    int err;
    double x, y;

    te_variable vars[] = {{"x", &x}};
    te_expr* expression = te_compile(strExpr, vars, 1, &err);
    if (!expression) { fprintf(stderr, "Error compiling expression %d\n", err); return EXIT_FAILURE; }

    int prev_sx = 0;
    int prev_sy = 0;
    bool first = true;

    SDL_SetRenderDrawColor(renderer, functionColor->r, functionColor->g, functionColor->b, functionColor->a);

    for (int sx = 0; sx < SCREEN_WIDTH; sx++) {
        x = screenToWorldX(camera, sx);
        y = te_eval(expression) * -1;
        int sy = worldToScreenY(camera, y);

        if (!first) {
            SDL_RenderDrawLine(renderer, prev_sx, prev_sy, sx, sy);
        }
        prev_sx = sx;
        prev_sy = sy;
        first = false;
    }

    te_free(expression);

    return EXIT_SUCCESS;
}

int worldToScreenX(Camera* cam, float wx) {
    return (int)((wx - cam->x) * cam->zoom + SCREEN_ORIGIN_X);
}

int worldToScreenY(Camera* cam, float wy) {
    return (int)((wy - cam->y) * cam->zoom + SCREEN_ORIGIN_Y);
}

double screenToWorldX(Camera* cam, int sx) {
    return (float)((sx - SCREEN_ORIGIN_X) / cam->zoom + cam->x);
}

double screenToWorldY(Camera* cam, int sy) {
    return (float)((sy - SCREEN_ORIGIN_Y) / cam->zoom + cam->x);
}

int initSDL(SDL_Window** window, SDL_Renderer** renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        return EXIT_FAILURE;
    }

    *window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!*window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!*renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void cleanupSDL(SDL_Window* window, SDL_Renderer* renderer, Text* zoomText, Text* exprText, Text* mouseText) {
    if (zoomText != NULL) {
        SDL_DestroyTexture(zoomText->texture);
        SDL_FreeSurface(zoomText->surface);
    } 
    if (exprText != NULL) {
        SDL_DestroyTexture(exprText->texture);
        SDL_FreeSurface(exprText->surface);
    }
    if (mouseText != NULL) {
        SDL_DestroyTexture(exprText->texture);
        SDL_FreeSurface(mouseText->surface);
    }

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}


void pollInput(SDL_Event* event, SDL_Window* window, bool* running, bool* zoomChanged, bool* mouseClicked, Camera* cam, char* buffer) {
    float move = 50 / cam->zoom;
    while (SDL_PollEvent(event)) {
        switch ((*event).type) {
            case SDL_WINDOWEVENT: {
                if ((*event).window.event == SDL_WINDOWEVENT_RESIZED) {
                    printf("Resizing window...\n");
                    int newWidth, newHeight;
                    SDL_GetWindowSize(window, &newWidth, &newHeight);
                    SCREEN_WIDTH = newWidth;
                    SCREEN_HEIGHT = newHeight;
                    SCREEN_ORIGIN_X = newWidth / 2;
                    SCREEN_ORIGIN_Y = newHeight / 2;
                }
                break;
            }
            case SDL_QUIT: {
                *running = false;
                break;
            }
            case SDL_KEYDOWN: {
                switch ((*event).key.keysym.sym) {
                    case SDLK_q: {
                        *running = false;
                        break;
                    }
                    case SDLK_UP: {
                        cam->zoom += 10.0f;
                        cam->zoom = roundf(cam->zoom);
                        if (cam->zoom > 1000.0f) cam->zoom = 1000.0f;
                        snprintf(buffer, 128, "Zoom: %.2f", cam->zoom);
                        *zoomChanged = true;
                        break;
                    }
                    case SDLK_DOWN: {
                        cam->zoom -= 10.0f;
                        cam->zoom = roundf(cam->zoom);
                        if (cam->zoom < 0.01f) cam->zoom = 0.01f;
                        snprintf(buffer, 128, "Zoom: %.2f", cam->zoom);
                        *zoomChanged = true;
                        break;
                    }
                    case SDLK_a: {
                        cam->x -= move;
                        break;
                    }
                    case SDLK_d: {
                        cam->x += move;
                        break;
                    }
                    case SDLK_w: {
                        cam->y -= move;
                        break;
                    }
                    case SDLK_s: {
                        cam->y += move;
                        break;
                    }
                    default: {
                        break;
                    }
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN: {
                *mouseClicked = true;
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                *mouseClicked = false;
                break;
            }
            case SDL_MOUSEWHEEL: {
                if (event->wheel.y > 0) {
                    cam->zoom *= 1.1f;
                    if (cam->zoom > 1000.0f) cam->zoom = 1000.0f;
                    snprintf(buffer, 128, "Zoom: %.2f", cam->zoom);
                    *zoomChanged = true;
                } else if (event->wheel.y < 0) {
                    cam->zoom *= 0.9f;
                    if (cam->zoom < 0.01f) cam->zoom = 0.01f;
                    snprintf(buffer, 128, "Zoom: %.2f", cam->zoom);
                    *zoomChanged = true;
                } 
                break;
            }
            case SDL_KEYUP: {
                break;
            }
            default: {
                break;
            }
        }
    }
}