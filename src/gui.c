#include "editor.h"
#ifdef GUI
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static TTF_Font *font;

// Function to write a string to the screen
int
gui_write_string(const char *str, U32 c, int x, int y) {
    SDL_Surface *surface = NULL;
    SDL_Texture *texture = NULL;
    SDL_Rect rect;
    SDL_Color color;

    // Set color
    color.r = (c >> 16) & 0xff;
    color.g = (c >> 8) & 0xff;
    color.b = c & 0xff;
    color.a = 0xff;

    // Create surface and texture from string
    surface = TTF_RenderText_Solid(font, str, color);
    if(surface == NULL) {
        fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
        return -1;
    }
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if(texture == NULL) {
        fprintf(stderr, "SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
        return -1;
    }

    // Set position and dimensions
    rect.x = x;
    rect.y = y;
    rect.w = surface->w;
    rect.h = surface->h;

    // Render texture to screen
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    // Clean up
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
	return 0;
}

__attribute__((constructor)) static int
init_gui(void)
{
    // Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return -1;
    }
    // Initialize TTF
    if(TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        return -1;
    }

    // Load font
    font = TTF_OpenFont("font.ttf", 24);
    if(font == NULL) {
        fprintf(stderr, "TTF_OpenFont Error: %s\n", TTF_GetError());
        return -1;
    }

    // Create window and renderer
    window = SDL_CreateWindow("Write String Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
    if(window == NULL) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        return -1;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        return -1;
    }
	return 0;
}

__attribute__((destructor)) static void
uninit_gui(void)
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
	SDL_Quit();
}
#endif
