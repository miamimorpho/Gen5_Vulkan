// tested with spleen-8x16-437-custom.bdf
#include "vulkan_public.h"
#include <stdint.h>

typedef struct{
  uint32_t glyph_c;
  uint32_t glyph_width;
  uint32_t height;
  uint8_t* pixels;
  uint32_t texture_index;
} GfxFont;

typedef struct{
  vec2 pos;
  vec2 uv;
} vertex2;

int gfx_font_create(GfxContext, char*, GfxFont*);
int font_free(GfxFont* font);
int render_3D_text(GfxContext context, GfxFont font, char* string,
		   GfxBuffer* dest, GfxModelOffsets* model);
