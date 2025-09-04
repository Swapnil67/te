#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL.h>

#include "la.h"
#include "editor.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define FONT_WIDTH 128
#define FONT_HEIGHT 64
#define FONT_COLS 18
#define FONT_ROWS 7
#define FONT_CHAR_WIDTH  (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT (FONT_HEIGHT / FONT_ROWS)
#define FONT_SCALE 5

void scc(int code) {
  if (code < 0) {
    fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
    exit(1);
  }
}

void *scp(void *ptr) {
  if (ptr == NULL) {
    fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
    exit(1);
  }
  return ptr;
}

const int WIDTH = 800;
const int HEIGHT = 600;

// * Create an SDL_Surface from a image
SDL_Surface *surface_from_file(const char *filepath) {
  int width, height, n;
  int req_format = STBI_rgb_alpha;
  unsigned char *pixels = stbi_load(filepath, &width, &height, &n, req_format);
  if (pixels == NULL) {
    fprintf(stderr, "ERROR: could not load file %s: %s\n", filepath, stbi_failure_reason());
  }

  
  Uint32 rmask, gmask, bmask, amask;
  /* SDL interprets each pixel as a 32-bit number, so our masks must depend
     on the endianness (byte order) of the machine */
  #if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
  #else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
  #endif

  const int depth = 32;
  const int pitch = 4 * width;

  return scp(SDL_CreateRGBSurfaceFrom((void *)pixels,
                                      width, height,
                                      depth, pitch,
                                      rmask, gmask, bmask, amask));
}

#define ASCII_DISPLAY_LOW 32
#define ASCII_DISPLAY_HIGH 126

typedef struct {
  SDL_Texture *spritesheet;
  SDL_Rect glyph_table[ASCII_DISPLAY_HIGH - ASCII_DISPLAY_LOW + 1];
} Font;

Font font_load_from_file(SDL_Renderer *renderer, const char *file_path) {
  Font font = {0};

  SDL_Surface *font_surface = surface_from_file(file_path);
  scc(SDL_SetColorKey(font_surface, SDL_TRUE, 0xFF000000)); // * transparent color

  font.spritesheet = scp(SDL_CreateTextureFromSurface(renderer, font_surface));

  // * Free the SDL_Surface
  SDL_FreeSurface(font_surface);

  // * Pre calculate all the glyphs rects
  for (size_t ascii = ASCII_DISPLAY_LOW; ascii <= ASCII_DISPLAY_HIGH; ++ascii) {
    const size_t index = ascii - ASCII_DISPLAY_LOW;
    const size_t col = index % FONT_COLS;
    const size_t row = index / FONT_COLS;

    // printf("index: %zu, col: %zu, row: %zu\n", index, col, row);

    font.glyph_table[index] = (SDL_Rect){
        .x = col * FONT_CHAR_WIDTH,
        .y = row * FONT_CHAR_HEIGHT,
        .w = FONT_CHAR_WIDTH,
        .h = FONT_CHAR_HEIGHT};
  }

  return font;
}

void render_char(SDL_Renderer *renderer,
                 const Font *font,
                 char c,
                 Vec2f pos,
                 float scale)
{

  const SDL_Rect dst = {
      .x = (int)floorf(pos.x),
      .y = (int)floorf(pos.y),
      .w = (int)floorf(FONT_CHAR_WIDTH * scale),
      .h = (int)floorf(FONT_CHAR_HEIGHT * scale)};

  assert(c >= ASCII_DISPLAY_LOW);
  assert(c <= ASCII_DISPLAY_HIGH);
  const size_t index = c - ASCII_DISPLAY_LOW;
  scc(SDL_RenderCopy(renderer, font->spritesheet, &font->glyph_table[index], &dst));
}

void set_texture_color(SDL_Texture *texture, Uint32 color) {
  // * set texture rgb color
  SDL_SetTextureColorMod(texture,
                         (color >> (8 * 0) & 0xFF),
                         (color >> (8 * 1) & 0xFF),
                         (color >> (8 * 2) & 0xFF));

  // * set texture alpha color
  scc(SDL_SetTextureAlphaMod(texture, (color >> (8 * 3) & 0xFF)));
}

void render_text_sized(SDL_Renderer *renderer,
                       Font *font,
                       const char *text,
                       size_t text_size,
                       Vec2f pos,
                       Uint32 color,
                       float scale)
{

  set_texture_color(font->spritesheet, color);
  Vec2f pen = pos;
  for (size_t i = 0; i < text_size; ++i) {
    // printf("text[i]: %c\n", text[i]);
    render_char(renderer, font, text[i], pen, scale);
    pen.x += FONT_CHAR_WIDTH * scale;
  }
}


// #define BUFFER_CAPACITY 1024
// char buffer[BUFFER_CAPACITY];
// size_t buffer_cursor = 0;
// size_t buffer_size = 0;

Editor editor = {0};

#define UNHEX(color)               \
  ((color) >> (8 * 0)) & 0xFF,     \
      ((color) >> (8 * 1)) & 0xFF, \
      ((color) >> (8 * 2)) & 0xFF, \
      ((color) >> (8 * 3)) & 0xFF

// * Renders the cursor
void render_cursor(SDL_Renderer *renderer, const Font* font, Uint32 color) {
  const Vec2f pos = vec2f(
      (float)editor.cursor_col * FONT_CHAR_WIDTH * FONT_SCALE,
      (float)editor.cursor_row * FONT_CHAR_HEIGHT * FONT_SCALE);

  const SDL_Rect rect = {
      .x = (int)floorf(pos.x),
      .y = (int)floorf(pos.y),
      .w = FONT_CHAR_WIDTH * FONT_SCALE,
      .h = FONT_CHAR_HEIGHT * FONT_SCALE};

  scc(SDL_SetRenderDrawColor(renderer, UNHEX(color)));
  scc(SDL_RenderFillRect(renderer, &rect));

  
  // * Render the overlapping character on cursor rect
  const char *c = editor_char_under_cursor(&editor);
  if (c) {
    // * set the font texture color to black
    set_texture_color(font->spritesheet, 0xFF000000);
    render_char(renderer, font, *c, pos, FONT_SCALE);
  }
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  scc(SDL_Init(SDL_INIT_VIDEO));

  SDL_Window *window = scp(SDL_CreateWindow("Text Editor",
                                            0, 0, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE));

  SDL_Renderer *renderer = scp(SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC));

  const char *file_path = "./charmap-oldschool_white.png";
  Font font = font_load_from_file(renderer, file_path);

  // * Event loop
  bool quit = false;
  while(!quit) {
    SDL_Event event = {0};
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT: {
          quit = true;
        } break;

        case SDL_KEYDOWN: {
          switch (event.key.keysym.sym) {
            // * Handle Backspace
            case SDLK_BACKSPACE: {
              editor_backspace(&editor);
            } break;
            
            case SDLK_F2: {
              editor_save_to_file(&editor, "output.txt");
            } break;
            
            case SDLK_RETURN: {
              editor_insert_new_line(&editor);
            } break;
            
            case SDLK_UP: {
              if (editor.cursor_row > 0) {
                editor.cursor_row -= 1;
              }
            } break;
            
            case SDLK_DOWN: {
              editor.cursor_row += 1;
            } break;
            
            case SDLK_DELETE: {
              editor_delete(&editor);
            } break;

            case SDLK_LEFT: {
              if (editor.cursor_col > 0)
                editor.cursor_col -= 1;
              } break;

            case SDLK_RIGHT: {
              editor.cursor_col += 1;
            } break;
          }
        } break;

        case SDL_TEXTINPUT: {
          editor_insert_text_before_cursor(&editor, event.text.text);
        } break;
      }
    }
    
    scc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
    scc(SDL_RenderClear(renderer));
    
    // SDL_RenderCopy(renderer, font.spritesheet, &src, &dst);
    for (size_t row = 0; row < editor.size; ++row) {
      const Line *line = editor.lines + row;
      // printf("text: %s\n", line->chars);
      render_text_sized(renderer, &font,
                        line->chars,
                        line->size,
                        vec2f(0.0f, row * FONT_CHAR_HEIGHT * FONT_SCALE),
                        0xFFFFFFFF, FONT_SCALE);
    }
    render_cursor(renderer, &font, 0xFFFFFFFF);

    SDL_RenderPresent(renderer);
  }

  SDL_Quit();
  return 0;
}
