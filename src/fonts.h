// tested with spleen-8x16-437-custom.bdf
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct{
  int glyph_c;
  int glyph_width;
  int height;
  uint8_t* pixels;
} GfxFont;

int bdf_load(GfxFont* font, char* filename);

int font_free(GfxFont* font);
