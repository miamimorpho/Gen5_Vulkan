#include "fonts.h"
#define L_LEN 80
#define W_LEN 8

int bdf_load(GfxFont* font, char* filename){

  FILE *fp = fopen(filename, "r");
  if(fp == NULL) {
    printf("Error opening font file\n");
    return 0;
  }
 
  char line[L_LEN];
  char setting[L_LEN];
  
  int supported = 0;
  /* METADATA LOADING START */
  while(fgets(line, sizeof(line), fp) != NULL) {

    if(supported == 3)break;
    
    sscanf(line, " %s", setting);

    if(strcmp(setting, "FONTBOUNDINGBOX") == 0){

      char x_s[W_LEN];
      char y_s[W_LEN];
      char x_offset_s[W_LEN];
      char y_offset_s[W_LEN];
      
      sscanf(line, "%*s %s %s %s %s",
	     x_s, y_s, x_offset_s, y_offset_s);

      font->glyph_width = atoi(x_s);
      font->height = atoi(y_s);
  
      supported += 1;
      continue;
    }

    if(strcmp(setting, "CHARSET_ENCODING") == 0){
      char encoding[W_LEN]; 
      
      sscanf(line, "%*s %s", encoding);
      if(strcmp(encoding, "\"437\"") == 0 )printf("Supported Encoding!\n");

      supported += 1;
      continue;
    }

    if(strcmp(setting, "CHARS") == 0){
      char glyph_c_s[W_LEN];
      sscanf(line, "%*s %s", glyph_c_s);

      font->glyph_c = atoi(glyph_c_s);
      //printf("Glyph Count %d\n", font.glyph_c);
      supported += 1;
      continue;
    }
   
  }
  
  if(supported != 3){
    printf("not enough config information\n");
    return 1;
  }/* META LOADING END */

  int image_size = font->glyph_width
    * font->glyph_c
    * font->height;
  font->pixels = (uint8_t*)malloc(image_size);
  memset(font->pixels, 255, image_size);
  
  rewind(fp);
  int in_bytes = 0;
  int row_i = 0;
  int code_i = 0;

  /* BYTE LOADING START */
  while(fgets(line, sizeof(line), fp) != NULL) {
 
    sscanf(line, "%s", setting);

    if(strcmp(setting, "ENCODING") == 0){
      char code[W_LEN];
      sscanf(line, "%*s %s", code);
      code_i = atoi(code);
 
   
      continue;
    }
    
    if(strcmp(setting, "ENDCHAR") == 0){
      row_i = 0;
      code_i = 0;
      in_bytes = 0;
    
      continue;
    }
    
    if(in_bytes == 1){
   
      uint8_t row = (uint8_t)strtol(line, NULL, 16);
      int y = row_i++ * (font->glyph_width * font->glyph_c);
    
      for(signed int i = 7; i >= 0; --i){
	int pixel_xy = y + (code_i * font->glyph_width) + i;
	uint8_t pixel = 0;
	if((row >> (7 - i) ) & 0x01)pixel = 255;	
	font->pixels[pixel_xy] = pixel;
      }
   
      continue;
    }
      
    if(strcmp(setting, "STARTCHAR") == 0){
      char* glyph_name = strchr(line, ' ');
      if(glyph_name != NULL)glyph_name += 1;
      glyph_name[strlen(glyph_name)-1] = '\0';
 
      continue;
    }

    if(strcmp(setting, "BITMAP") == 0){
      in_bytes = 1;
   
      continue;
    }
  }/* BYTE LOADING END*/

  fclose(fp);
  return 0;
  
}

int font_free(GfxFont* font){
  free(font->pixels);
  return 0;
}
