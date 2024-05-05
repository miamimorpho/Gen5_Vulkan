// tested with spleen-8x16-437-custom.bdf
#include "vulkan_public.h"
#include <stdint.h>

typedef struct{
  uint32_t glyph_c;
  uint32_t glyph_width;
  uint32_t height;
  //uint8_t* pixels;
  uint32_t texture_index;
} GfxFont;

typedef struct{
  vec2 pos;
  vec2 uv;
} vertex2;

typedef struct {
  vertex2* vertices;
  uint32_t vertex_c;
  uint32_t* indices;
  uint32_t index_c;
} GfxStagingText;

typedef struct {
  vec2 pos;
  GfxModelOffsets model;
} text_entity;

int gfx_font_load(GfxContext context, GfxFont* font, char* filename);
int gui_shader_create(GfxContext context);
int gui_shader_bind(GfxContext context);

int text_render(GfxContext context, GfxFont font, char* string,
		   GfxBuffer* dest);

int font_free(GfxFont* font);
