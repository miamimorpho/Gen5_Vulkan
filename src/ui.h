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

typedef struct {
  vertex2* vertices;
  uint32_t vertex_c;
  uint32_t* indices;
  uint32_t index_c;
} GfxStagingText;

typedef struct{
  mat4 view_m;
  uint32_t texture_i;
} text_args;

typedef struct {
  vec2 pos;
  GfxModelOffsets model;
} text_entity;

int gfx_font_load(GfxContext context, GfxFont* font, char* filename);
int text_shader_create(GfxContext context, GfxShader* shader);
int text_indirect_b_create(GfxContext context, GfxShader* shader, size_t draw_count);
int text_render(GfxContext context, GfxFont font, char* string,
		   GfxBuffer* dest, GfxModelOffsets* model);
void text_draw(GfxContext context, GfxShader shader,
	       text_entity* entities, int count);
int font_free(GfxFont* font);
