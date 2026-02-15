#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "tinyexpr.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define SCREEN_ORIGIN_X (SCREEN_WIDTH / 2)
#define SCREEN_ORIGIN_Y (SCREEN_HEIGHT / 2)
#define WINDOW_TITLE "Math Plotter" 
#define FONT_PATH "assets/JetBrainsMono-Bold.ttf" 

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

void pollInput(SDL_Event* event, bool* running, bool* zoomChanged, bool* mouseClicked, Camera* cam, char* buffer);
void freeTextSurfaceAndTexture(SDL_Renderer* renderer, TTF_Font* font, Text* text);
int worldToScreenX(Camera* cam, float wx);
int worldToScreenY(Camera* cam, float wy);
double screenToWorldX(Camera* cam, int sx);
double screenToWorldY(Camera* cam, int sy);
void cleanupSDL(SDL_Window* window, SDL_Renderer* renderer, Text* zoomText, Text* exprText, Text* mouseText);
int initSDL(SDL_Window** window, SDL_Renderer** renderer);
void drawAxes(SDL_Renderer* renderer, Camera* camera);
int plotFunction(SDL_Renderer* renderer, Camera* camera, char* strExpr, SDL_Color* functionColor);


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
        if ((strcmp(argv[2], "BLUE")) == 0) functionColor = (SDL_Color){0, 0, 150, 255};
        else if ((strcmp(argv[2], "RED")) == 0) functionColor = (SDL_Color){235, 0, 0, 255};
        else if ((strcmp(argv[2], "BLACK")) == 0) functionColor = (SDL_Color){0, 0, 0, 255};
        else if ((strcmp(argv[2], "GREEN")) == 0) functionColor = (SDL_Color){0, 150, 0, 255};
        else if ((strcmp(argv[2], "YELLOW")) == 0) functionColor = (SDL_Color){200, 200, 0, 255};
    } else {
        functionColor = (SDL_Color){235, 0, 0, 255};
    }

    printf("Function color: %s\n", argv[2]);

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

    snprintf(zoomText.buffer, sizeof(zoomText.buffer), "Zoom: %f", camera.zoom);
    zoomText.color = (SDL_Color){0, 0, 0, 255};
    zoomText.surface = TTF_RenderText_Solid(font, zoomText.buffer, zoomText.color);
    zoomText.texture = SDL_CreateTextureFromSurface(renderer, zoomText.surface);
    zoomText.rect = (SDL_Rect){0, 0, 200, 50};

    snprintf(exprText.buffer, sizeof(zoomText.buffer), "f(x) = %s", argv[1]);
    exprText.color = (SDL_Color){0, 0, 0, 255};
    exprText.surface = TTF_RenderText_Solid(font, exprText.buffer, exprText.color);
    exprText.texture = SDL_CreateTextureFromSurface(renderer, exprText.surface);
    exprText.rect = (SDL_Rect){0, 50, 200, 50};

    mouseText.color = (SDL_Color){0, 0, 0, 255};
    mouseText.surface = TTF_RenderText_Solid(font, mouseText.buffer, mouseText.color);
    mouseText.texture = SDL_CreateTextureFromSurface(renderer, mouseText.surface);

    printf("Starting math plotter...\n");

    int smx, smy;
    int prev_smx, prev_smy;

    while (running) {
        SDL_GetMouseState(&smx, &smy);

        if (prev_smx != smx || prev_smy != smy) {
            snprintf(mouseText.buffer, sizeof(mouseText.buffer), "  (%f, %f)", screenToWorldX(&camera, smx), screenToWorldY(&camera, smy) * -1);
            freeTextSurfaceAndTexture(renderer, font, &mouseText);
            prev_smx = smx;
            prev_smy = smy;
        }

        mouseText.rect = (SDL_Rect){smx, smy, 300, 25};

        pollInput(&event, &running, &zoomChanged, &mouseClicked, &camera, zoomText.buffer);

        if (zoomChanged) {
            freeTextSurfaceAndTexture(renderer, font, &zoomText);

            snprintf(mouseText.buffer, sizeof(mouseText.buffer), "  (%f, %f)", screenToWorldX(&camera, smx), screenToWorldY(&camera, smy) * -1);
            freeTextSurfaceAndTexture(renderer, font, &mouseText);
        
            zoomChanged = false;
        }

        if (mouseClicked) {
            double x = screenToWorldX(&camera, smx);
            te_variable vars[] = {{"x", &x}};
            te_expr* expression = te_compile(argv[1], vars, 1, 0);
            snprintf(mouseText.buffer, sizeof(mouseText.buffer), "  For X = %f, Y = %f", x, te_eval(expression));
            freeTextSurfaceAndTexture(renderer, font, &mouseText);
        } else {
            snprintf(mouseText.buffer, sizeof(mouseText.buffer), "  (%f, %f)", screenToWorldX(&camera, smx), screenToWorldY(&camera, smy) * -1);
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
        SDL_RenderCopy(renderer, mouseText.texture, NULL, &mouseText.rect);

        SDL_RenderPresent(renderer);
    }   

    printf("Exiting math plotter...\n");

    cleanupSDL(window, renderer, &zoomText, &exprText, &mouseText);

    return EXIT_SUCCESS;
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
    gridStep = roundf(gridStep / 5.0f) * 5.0f;

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

    *window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
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

void pollInput(SDL_Event* event, bool* running, bool* zoomChanged, bool* mouseClicked, Camera* cam, char* buffer) {
    while (SDL_PollEvent(event)) {
        switch ((*event).type) {
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
                        snprintf(buffer, 128, "Zoom: %f", cam->zoom);
                        *zoomChanged = true;
                        break;
                    }
                    case SDLK_DOWN: {
                        cam->zoom -= 10.0f;
                        cam->zoom = roundf(cam->zoom);
                        if (cam->zoom < 0.1f) cam->zoom = 0.1f;
                        snprintf(buffer, 128, "Zoom: %f", cam->zoom);
                        *zoomChanged = true;
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
                    snprintf(buffer, 128, "Zoom: %f", cam->zoom);
                    *zoomChanged = true;
                } else if (event->wheel.y < 0) {
                    cam->zoom *= 0.9f;
                    if (cam->zoom < 0.1f) cam->zoom = 0.1f;
                    snprintf(buffer, 128, "Zoom: %f", cam->zoom);
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