#ifndef DRAWING_H
#define DRAWING_H

#include <cglm/cglm.h>
#include "vulkan_public.h"
#include "model.h"
#include "input.h"

/* Draw Commands solid_draw.c */
int draw_start(GfxContext*);

Entity entity_add1(GfxModelOffsets model, float x, float y, float z);

int
draw_end(GfxContext context);

#endif /* SOLID_H */
