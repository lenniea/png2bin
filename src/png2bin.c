
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <png.h>
#include <config.h>

#define FATAL_ERROR(...) 	printf(__VA_ARGS__); retval = 1; goto ExitProgram

//==================================================================================================
const char HELP[] =
	"Usage: png2bin [OPTION] FILE\n"
	"Options:\n"
	"  -r##       Outputs rotated image clockwise (0,90,180,270)\n"
	"  -o <file>  Place the output into <file>\n"
	;

char DEFAULT_OUT[] = "out.bin";
	
void RotateBinImage(uint8_t **pimg, uint32_t *w, uint32_t *h, int rotate);

//==================================================================================================
int main(int argc, char *argv[]){
	//--------------------------------------- Set Defaults -----------------------------------------
	char *strInFile = NULL;
	char *strOutFile = DEFAULT_OUT;
	int rotate = 0;
	
	//-------------------------------------- Interpret args ----------------------------------------
	int arg_idx = 1;
	while(arg_idx < argc){
		if(argv[arg_idx][0] == '-'){
			// is option
			switch(argv[arg_idx][1]){
				case 'r':
					rotate = atoi(&argv[arg_idx][2]);
					break;
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
	
	if((rotate != 0) && (rotate != 90) && (rotate != 180) && (rotate != 270)){
		printf("Error: Invalid rotation\n");
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
	
	if(!((bitdepth == 1) || (bitdepth == 8 && (colortype == PNG_COLOR_TYPE_RGB || colortype == PNG_COLOR_TYPE_RGB_ALPHA)))){
		FATAL_ERROR("Error: Only monochrome or 24-bit RGB PNGs are supported:\n depth=%d\n type=%d\n",bitdepth,colortype);
	}
	
	//printf("width = %d\n",width);
	//printf("height = %d\n",height);
	//printf("bitdepth = %d\n",bitdepth);
	//printf("colortype = %d\n",colortype);
	
	// generate binary image into a massive array --------------------------------------------------
	uint16_t wbyte, hbyte;
	if(width%8){
		wbyte = width/8+1;
	}else{
		wbyte = width/8;
	}
	hbyte = height;
	
	img = malloc(wbyte*hbyte);
	if(!img) {
		FATAL_ERROR("malloc fail at line %d\n",__LINE__);
	}
	
	if(bitdepth == 1){
		uint32_t xb,y;
		uint8_t mask;
		uint8_t mod;
		
		mod = width%8;
		if(mod){
			mask = 0x80;
			while(mod > 1){
				mask |= (mask >>1);
				mod--;
			}
		}else{
			mask = 0xFF;
		}
		
		for(y=0;y<height;y++){
			png_read_row(png_ptr,img+wbyte*y,NULL);
			for(xb=0;xb<wbyte;xb++){
				img[wbyte*y+xb] = ~img[wbyte*y+xb];
			}
			img[wbyte*y+wbyte-1] &= mask;
		}
	}else{
		uint8_t* png_row = NULL;
		uint8_t bpp;
		if(colortype == PNG_COLOR_TYPE_RGB){
			bpp = 3;
		}else if(colortype == PNG_COLOR_TYPE_RGB_ALPHA){
			bpp = 4;
		}
		png_row = malloc(width * bpp);
		
		memset(img,0,wbyte*hbyte);
		
		uint32_t x,y;
		size_t xb;
		uint8_t mask;
		for(y=0;y<height;y++){
			png_read_row(png_ptr,png_row,NULL);
			xb = 0;
			mask = 0x80;
			for(x=0;x<width;x++){
				if((png_row[x*bpp] == 0x00)&&(png_row[x*bpp+1] == 0x00)&&(png_row[x*bpp+2] == 0x00)){ // equals black
					// is dark
					img[wbyte*y+xb] |= mask;
				}else{
					// is light
					img[wbyte*y+xb] &= ~mask;
				}
				
				mask >>= 1;
				if(mask == 0){
					mask = 0x80;
					xb++;
				}
			}
		}
		
		if(png_row) free(png_row);
	}
	
	// rotate image if necessary -------------------------------------------------------------------
	RotateBinImage(&img,&width,&height,rotate);
	if(width%8){
		wbyte = width/8+1;
	}else{
		wbyte = width/8;
	}
	hbyte = height;
	
	// Write output file ---------------------------------------------------------------------------
	fileOut = fopen(strOutFile,"wb");
	if(fileOut == NULL){
		FATAL_ERROR("Error: Could not create \"%s\"\n",strOutFile);
	}
	
	fwrite(img,wbyte*hbyte,1,fileOut);
	
	//------------------------------- Close all files, Free memory ---------------------------------
	ExitProgram:
	if(fileIn) fclose(fileIn);
	if(fileOut) fclose(fileOut);
	if(img) free(img);
	if(info_ptr) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if(png_ptr) png_destroy_read_struct(&png_ptr, (png_infopp)NULL,(png_infopp)NULL);
	
	return(retval);
}

//==================================================================================================

void RotateBinImage(uint8_t **pimg, uint32_t *w, uint32_t *h, int rotate){
	uint16_t wbyte, hbyte;
	uint16_t wbyte2, hbyte2;
	uint32_t w2, h2;
	uint8_t* img2 = NULL;
	uint8_t* img = *pimg;
	uint32_t rx,ry,wx,wy;
	uint8_t rmask,wmask;
	
	// make a copy of img into img2
	w2 = *w;
	h2 = *h;
	if(w2%8){
		wbyte2 = w2/8+1;
	}else{
		wbyte2 = w2/8;
	}
	hbyte2 = h2;
	img2 = malloc(wbyte2*hbyte2);
	memcpy(img2,img,wbyte2*hbyte2);
	
	//temp:
	wbyte = wbyte2;
	hbyte = hbyte2;
	
	switch(rotate){
		case 90:
			*h = w2;
			*w = h2;
			
			if(*w%8){
				wbyte = *w/8+1;
			}else{
				wbyte = *w/8;
			}
			hbyte = *h;
			
			free(img);
			img = malloc(wbyte*hbyte);
			memset(img,0,wbyte*hbyte);
			
			for(rx=0,wy=0; rx<w2; rx++,wy++){
				rmask = 0x80 >> (rx%8);
				for(ry=0,wx=*w-1; ry<h2; ry++,wx--){
					wmask = 0x80 >> (wx%8);
					if(img2[wbyte2*ry+(rx/8)] & rmask){
						img[wbyte*wy+(wx/8)] |= wmask;
					}else{
						img[wbyte*wy+(wx/8)] &= ~wmask;
					}
				}
			}
			break;
		case 180:
			wbyte = wbyte2;
			hbyte = hbyte2;
			memset(img,0,wbyte*hbyte);
			
			for(rx=0,wx=*w-1; rx<w2; rx++,wx--){
				rmask = 0x80 >> (rx%8);
				wmask = 0x80 >> (wx%8);
				for(ry=0,wy=*h-1; ry<h2; ry++,wy--){
					if(img2[wbyte2*ry+(rx/8)] & rmask){
						img[wbyte*wy+(wx/8)] |= wmask;
					}else{
						img[wbyte*wy+(wx/8)] &= ~wmask;
					}
				}
			}

			break;
		case 270:
			*h = w2;
			*w = h2;
			
			if(*w%8){
				wbyte = *w/8+1;
			}else{
				wbyte = *w/8;
			}
			hbyte = *h;
			free(img);
			img = malloc(wbyte*hbyte);
			memset(img,0,wbyte*hbyte);
			
			for(rx=0,wy=*h-1; rx<w2; rx++,wy--){
				rmask = 0x80 >> (rx%8);
				for(ry=0,wx=0; ry<h2; ry++,wx++){
					wmask = 0x80 >> (wx%8);
					if(img2[wbyte2*ry+(rx/8)] & rmask){
						img[wbyte*wy+(wx/8)] |= wmask;
					}else{
						img[wbyte*wy+(wx/8)] &= ~wmask;
					}
				}
			}
			
			
			
			break;
	}
	
	if(img2) free(img2);
	
	*pimg = img;
	
	
	#if 0 // disabled. only for debug
	// print the image to the terminal:
	int i;
	int x,y;
	
	for(y=0;y<hbyte;y++){
		for(x=0;x<wbyte;x++){
			rmask = 0x80;
			for(i=0;i<8;i++){
				if(img[y*wbyte+x] & rmask){
					printf("# ");
				}else{
					printf("  ");
				}
				rmask >>=1;
			}
		}
		printf("\n");
	}
	#endif
	
}


