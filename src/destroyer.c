#include "destroyer.h"
#include <stddef.h>

typedef struct {
  void (*func)(GfxContext context, void*);
  void* arg1;
} defer_func_t;

static size_t defer_q_size = 0;
static size_t defer_q_offset = 0;
static defer_func_t* defer_q = NULL;

int deferd_add( void (*func)(GfxContext, void*), void* arg1){
  if(defer_q_offset >= defer_q_size){
    size_t new_size = (defer_q_size == 0) ? 1 : defer_q_size * 2;
    defer_func_t* new_buffer = realloc(defer_q,
				     new_size * sizeof(defer_func_t));
    if(!new_buffer) {
      return 1;
    }
    defer_q = new_buffer;
    defer_q_size = new_size;
  }
  defer_q[defer_q_offset].func = func;
  defer_q[defer_q_offset].arg1 = arg1;
  defer_q_offset++;
  
  return 0;
}

int deferd_run(GfxContext context){
  for(ssize_t i = defer_q_offset -1; i >= 0; i--){
    printf("DEFER RUN%ld\n", i);
    defer_q[i].func(context, defer_q[i].arg1);
  }
  defer_q_offset = 0;
  return 0;
}

void deferd_buffer(GfxContext context, void* src_buffer){
  printf("deferd BUFFER\n");
  buffer_destroy(context, *(GfxBuffer*)src_buffer);
}

void deferd_image(GfxContext context, void* src){
  printf("deferd IMAGE\n");
  image_destroy(context, *(GfxImage*)src);
}

void deferd_VkPipelineLayout(GfxContext context, void* src){
  printf("deferd PIPELINE LAYOUT\n");
  vkDestroyPipelineLayout(context.l_dev, *(VkPipelineLayout*)src, NULL);
}

void deferd_VkPipeline(GfxContext context, void* src){
  printf("deferd PIPELINE\n");
  vkDestroyPipeline(context.l_dev, *(VkPipeline*)src, NULL);
}
