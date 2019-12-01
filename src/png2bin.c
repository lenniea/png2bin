
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <png.h>
//#include <config.h>

#define FATAL_ERROR(...) 	printf(__VA_ARGS__); retval = 1; goto ExitProgram

//==================================================================================================
const char HELP[] =
	"Usage: png2bin [OPTION] FILE\n"
	"Options:\n"
	"  -o <file>  Place the output into <file>\n"
	;

char DEFAULT_OUT[] = "out.bin";
	
//==================================================================================================
int main(int argc, char *argv[]){
	//--------------------------------------- Set Defaults -----------------------------------------
	char *strInFile = NULL;
	char *strOutFile = DEFAULT_OUT;
	
	//-------------------------------------- Interpret args ----------------------------------------
	int arg_idx = 1;
	while(arg_idx < argc){
		if(argv[arg_idx][0] == '-'){
			// is option
			switch(argv[arg_idx][1]){
				case 'o':
					arg_idx++;
					
					strOutFile = argv[arg_idx];
					
					if(arg_idx == argc){
						printf("Error: Argument to '%c' is missing\n",argv[arg_idx-1][1]);
						return(1);
					}
					
					break;
				case '-':{ //String options
					if(!strcmp(argv[arg_idx],"--help")){
						printf(HELP);
						return(0);
					}else{
						printf("Error: Unrecognized option \"%s\"\n",argv[arg_idx]);
						return(1);
					}
					break;
				}
				default:
					printf("Error: Unrecognized option '%c'\n",argv[arg_idx][1]);
					return(1);
			}
		}else{
			// is filename
			strInFile = argv[arg_idx];
		}
		arg_idx++;
	}
	
	//--------------------------------- Check for errors in args -----------------------------------
	if(!strInFile){
		printf("Error: No input file\n");
		return(1);
	}
	
	//--------------------------------------- Start Program ----------------------------------------
	int retval = 0;
	FILE *fileIn = NULL;
	FILE *fileOut = NULL;
	uint8_t* img = NULL;
	
	// open input file
	fileIn = fopen(strInFile,"rb");
	if(fileIn == NULL){
		FATAL_ERROR("Error: Could not open \"%s\"\n",strInFile);
	}
	
	// Verify that input is a valid PNG
	uint8_t header[8];
	int is_png;
	fread(header,1,sizeof(header),fileIn);
	is_png = !png_sig_cmp(header,0,sizeof(header));
	if(!is_png){
		FATAL_ERROR("Error: %s is not a valid PNG file\n",strInFile);
	}
	fseek(fileIn,0,SEEK_SET);
	
	// Initialize PNG input
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 
												 (png_voidp)NULL,NULL,NULL);
	if (!png_ptr){
		retval = 1;
		goto ExitProgram;
	}
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr){
		png_destroy_read_struct(&png_ptr,(png_infopp)NULL, (png_infopp)NULL);
		retval = 1;
		goto ExitProgram;
	}
	png_infop end_info = png_create_info_struct(png_ptr);
	if (!end_info){
		png_destroy_read_struct(&png_ptr, &info_ptr,(png_infopp)NULL);
		retval = 1;
		goto ExitProgram;
	}
	png_init_io(png_ptr,fileIn);
	
	if (setjmp(png_jmpbuf(png_ptr))) {
		FATAL_ERROR("Error during png read\n");
	}
	
	
	// get info about the PNG ---------------------------------------------------------------------
	uint32_t width, height;
	png_read_info(png_ptr,info_ptr);
	width = png_get_image_width(png_ptr,info_ptr);
	height = png_get_image_height(png_ptr,info_ptr);
	uint8_t bitdepth;
	bitdepth = png_get_bit_depth(png_ptr,info_ptr);
	uint8_t colortype;
	colortype = png_get_color_type(png_ptr,info_ptr);
	
	printf("dimentions = %d x %d\n", width, height);
	printf("bitdepth = %d  colortype=%d\n", bitdepth, colortype);

	if(!((bitdepth == 8 && (colortype == PNG_COLOR_TYPE_RGB || colortype == PNG_COLOR_TYPE_RGB_ALPHA))
	   || (bitdepth == 16))) {
		FATAL_ERROR("Error: Only 8-bit, 16-bit or 24-bit RGB PNGs are supported:\n depth=%d\n type=%d\n", bitdepth, colortype);
	}
	
	
	uint8_t* png_row = NULL;
	uint8_t bpp;
	if (bitdepth == 16) {
		bpp = 2;
	} else if (colortype == PNG_COLOR_TYPE_RGB){
		bpp = 3;
	} else if (colortype == PNG_COLOR_TYPE_RGB_ALPHA){
		bpp = 4;
	}
	png_row = malloc(width * bpp);
	
	// generate binary image into a massive array --------------------------------------------------
	uint16_t wbyte = width * bpp;
	
	img = malloc(wbyte*height);
	if(!img) {
		FATAL_ERROR("malloc fail at line %d\n",__LINE__);
	}
	
	memset(img,0,wbyte*height);
	
	uint32_t x,y;
	size_t xb;
	for (y=0; y<height; y++){
		png_read_row(png_ptr,png_row,NULL);
		xb = 0;
		uint32_t index = 0;
		uint32_t base = y * wbyte;
		for (x=0; x<width; x++){
			switch (bpp) {
			case 4:
				img[base + index] = png_row[index];
				++index;
			case 3:
				img[base + index] = png_row[index];
				++index;
			case 2:
				img[base+ index] = png_row[index];
				++index;
			default:
				img[base + index] = png_row[index];
				++index;
			}
		}
	}
	
	if(png_row) free(png_row);
	
	// Write output file ---------------------------------------------------------------------------
	fileOut = fopen(strOutFile,"wb");
	if(fileOut == NULL){
		FATAL_ERROR("Error: Could not create \"%s\"\n",strOutFile);
	}
	
	printf("writing %d x %d = %d bytes\n", wbyte, height, wbyte * height);
	fwrite(img, wbyte*height, 1, fileOut);
	
	//------------------------------- Close all files, Free memory ---------------------------------
	ExitProgram:
	if(fileIn) fclose(fileIn);
	if(fileOut) fclose(fileOut);
	if(img) free(img);
	if(info_ptr) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if(png_ptr) png_destroy_read_struct(&png_ptr, (png_infopp)NULL,(png_infopp)NULL);
	
	return(retval);
}