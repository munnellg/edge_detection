#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL.h>

#define SOBEL_MATRIX_WIDTH 3
#define SOBEL_MATRIX_HEIGHT 3

#define PREWITT_MATRIX_WIDTH 3
#define PREWITT_MATRIX_HEIGHT 3

#define ROBERTS_MATRIX_WIDTH 2
#define ROBERTS_MATRIX_HEIGHT 2

#define SCHARR_MATRIX_WIDTH 3
#define SCHARR_MATRIX_HEIGHT 3

#define MATRIX_MAX_WIDTH 3
#define MATRIX_MAX_HEIGHT 3

const char* CONVOLUTION_NAMES[] = {
	"Sobel (3x3)",
	"Prewitt (3x3)",
	"Roberts (2x2)",
	"Scharr (3x3)"
};

const char* NORMALIZATION_NAMES[] = {
	"Local Normalization",
	"Global Normalization",
	"No Normalization",
};

const char* DISPLAY_NAMES[] = {
	"Original",
	"Edges",
};

typedef enum {
	SHOW_ORIGINAL,
	SHOW_EDGES
} DisplayMode;

typedef enum {
	SOBEL,
	PREWITT,
	ROBERTS,
	SCHARR,
	NUM_MATRICES
} ConvolutionMode;

typedef enum {
	LOCAL,
	GLOBAL,
	NONE
} NormalizationMode;

struct ConvolutionMatrix {
	int width, height;
	int x_kernel[MATRIX_MAX_HEIGHT][MATRIX_MAX_WIDTH];
	int y_kernel[MATRIX_MAX_HEIGHT][MATRIX_MAX_WIDTH];
};

struct EdgeDetector {
	struct ConvolutionMatrix matrix;
	SDL_Surface* image;
	SDL_Surface* edges;
};

struct Display {
	SDL_Window* window;
	SDL_Renderer* renderer;
};
	
struct ImageProcessor {
	DisplayMode display_mode;
	ConvolutionMode convolution_mode;
	NormalizationMode normalization_mode;
	struct EdgeDetector* edge_detector;
	struct Display* display;
};

void die( struct ImageProcessor* ip, const char* message );
void initialize_sdl( void );
void handle_events( struct ImageProcessor* ip, int *quit );
void terminate_sdl( void );
Uint32 get_pixel( SDL_Surface *surface, int x, int y );
void set_pixel( SDL_Surface *surface, int x, int y, Uint32 pixel );
Uint32 to_greyscale( SDL_PixelFormat* format, Uint32 r, Uint32 g, Uint32 b, Uint32 a );
void explode( SDL_PixelFormat* format, Uint32 pixel, Uint32* r, Uint32* g, Uint32* b, Uint32* a );
Uint32 compress( SDL_PixelFormat* format, Uint32 r, Uint32 g, Uint32 b, Uint32 a );

struct ImageProcessor* ImageProcessor_create( void );
void ImageProcessor_load( struct ImageProcessor* ip, char* filename );
void ImageProcessor_set_normalization_mode ( struct ImageProcessor* ip, NormalizationMode nm );
void ImageProcessor_set_display_mode ( struct ImageProcessor* ip, DisplayMode dm );
void ImageProcessor_set_convolution_matrix ( struct ImageProcessor* ip, ConvolutionMode cm );
void ImageProcessor_update_title ( struct ImageProcessor* ip );
void ImageProcessor_detect( struct ImageProcessor* ip );
void ImageProcessor_show( struct ImageProcessor* ip );
void ImageProcessor_destroy( struct ImageProcessor* ip );

struct EdgeDetector* EdgeDetector_create( void );
void EdgeDetector_detect_normalize_local( struct EdgeDetector* ed );
void EdgeDetector_detect_normalize_global( struct EdgeDetector* ed );
void EdgeDetector_detect_normalize_none( struct EdgeDetector* ed );
void EdgeDetector_destroy( struct EdgeDetector* ed );

struct Display* Display_create( void );
void Display_destroy( struct Display *d );

void SobelConvolutionMatrix_init( struct ConvolutionMatrix* matrix );
void RobertsConvolutionMatrix_init( struct ConvolutionMatrix* matrix );
void PrewittConvolutionMatrix_init( struct ConvolutionMatrix* matrix );
void ScharrConvolutionMatrix_init( struct ConvolutionMatrix* matrix );
Uint32 ConvolutionMatrix_convolve ( struct ConvolutionMatrix* matrix, SDL_Surface* image, int x, int y );
Uint32 ConvolutionMatrix_max_output( struct ConvolutionMatrix* matrix, Uint32 max_input );

int main( int argc, char *argv[] ) {
	
	if( argc < 2 ) {
		die(NULL, "USAGE: edges <image>");
	}

	initialize_sdl();

	char* filename = argv[1];
	struct ImageProcessor* ip = ImageProcessor_create( );

	ImageProcessor_load(ip, filename);
	ImageProcessor_detect(ip);
	ImageProcessor_update_title ( ip );
	ImageProcessor_show(ip);
	
	int quit = 0;
	while(!quit) {
		handle_events(ip, &quit);
	}
	
	ImageProcessor_destroy( ip );
	terminate_sdl();
	
	return EXIT_SUCCESS;
}

void die( struct ImageProcessor* ip, const char* message ) {
	
	printf("ERROR: %s\n", message);
	
	ImageProcessor_destroy(ip);
	terminate_sdl();
	exit(1);
}

void initialize_sdl( void ) {
	/* only need to set up the display elements. We're not doing
	 * anything fancy with sound effects or music etc. */
	if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		die( NULL, SDL_GetError() );
	}

	/* going to allow the user to load as many different image types as
	 * possible */
	if( IMG_Init(IMG_INIT_TIF | IMG_INIT_PNG | IMG_INIT_JPG) < 0 ) {
		die( NULL, IMG_GetError() );
	}
}

void handle_events ( struct ImageProcessor* ip, int *quit ) {
	SDL_Event e;
	SDL_Scancode key;
	
	while( SDL_PollEvent(&e) ) {
		switch(e.type) {
		case SDL_QUIT:
			*quit = 1;
			break;
		case SDL_KEYDOWN:
			key = e.key.keysym.sym;
			if(key == 'o' || key == 'e') {
				ImageProcessor_set_display_mode( ip, (key == 'o')? SHOW_ORIGINAL : SHOW_EDGES );
				ImageProcessor_update_title ( ip );
				ImageProcessor_show( ip );
			} else if ( key - '1' >= 0 && key-'1' < NUM_MATRICES ){
				ImageProcessor_set_convolution_mode( ip, key - '1' );
				ImageProcessor_detect( ip );
				ImageProcessor_update_title ( ip );
				ImageProcessor_show(ip);
			} else if ( key == 'g' || key == 'l' || key == 'n' ) {
				ImageProcessor_set_normalization_mode( ip, (key == 'l')? LOCAL : ((key=='n')? NONE : GLOBAL)  );
				ImageProcessor_detect( ip );
				ImageProcessor_update_title ( ip );
				ImageProcessor_show(ip);
			}
			break;
		default:
			break;
		}
	}
}

void terminate_sdl ( void ) {
	IMG_Quit();
	SDL_Quit();
}

Uint32 get_pixel( SDL_Surface *surface, int x, int y ) {
	Uint32 *target_pixel = (Uint32 *) (surface->pixels + (y * surface->pitch + x * surface->format->BytesPerPixel));
	return *target_pixel;
}

void set_pixel( SDL_Surface *surface, int x, int y, Uint32 pixel ) {
	Uint32 *target_pixel = (Uint32 *) (surface->pixels + (y * surface->pitch + x * surface->format->BytesPerPixel));
	*target_pixel = pixel;
}

Uint32 to_greyscale( SDL_PixelFormat* format, Uint32 r, Uint32 g, Uint32 b, Uint32 a ) {
	return (r+g+b)/3;
}

void explode( SDL_PixelFormat* format, Uint32 pixel, Uint32* r, Uint32* g, Uint32* b, Uint32* a ) {
	*r = (pixel & format->Rmask) >> format->Rshift;
	*g = (pixel & format->Gmask) >> format->Gshift;
	*b = (pixel & format->Bmask) >> format->Bshift;
	*a = (pixel & format->Amask) >> format->Ashift;
}

Uint32 compress( SDL_PixelFormat* format, Uint32 r, Uint32 g, Uint32 b, Uint32 a ) {
	Uint32 pixel;
	pixel = 0;

	pixel |= ( r << format->Rshift );
	pixel |= ( g << format->Gshift );
	pixel |= ( b << format->Bshift );
	pixel |= ( a << format->Ashift );
	
	return pixel;
}

struct ImageProcessor* ImageProcessor_create( void ) {
	struct ImageProcessor* ip = malloc(sizeof(struct ImageProcessor));
	if(!ip) {
		die(ip, "Memory error");
	}

	ip->edge_detector = EdgeDetector_create( );	
	if(!ip->edge_detector) {
		die(ip, "Memory error");
	}
	
	ip->display = Display_create();
	if(!ip->display) {
		die(ip, "Memory error");
	}
	
	return ip;
}

void ImageProcessor_set_normalization_mode ( struct ImageProcessor* ip, NormalizationMode nm ) {
	ip->normalization_mode = nm;
}

void ImageProcessor_set_display_mode ( struct ImageProcessor* ip, DisplayMode dm ) {
	ip->display_mode = dm;
}

void ImageProcessor_set_convolution_mode ( struct ImageProcessor* ip, ConvolutionMode cm ) {
	ip->convolution_mode = cm;
	switch(ip->convolution_mode) {
	case SOBEL:
		SobelConvolutionMatrix_init( &ip->edge_detector->matrix);
		break;
	case ROBERTS:
		RobertsConvolutionMatrix_init( &ip->edge_detector->matrix);
		break;
	case PREWITT:
		PrewittConvolutionMatrix_init( &ip->edge_detector->matrix);
		break;
	case SCHARR:
		ScharrConvolutionMatrix_init( &ip->edge_detector->matrix);
		break;
	default:
		return;
	}
}

void ImageProcessor_update_title ( struct ImageProcessor* ip ) {
	char buf[128] = {0};

	sprintf(buf, "%s - %s - %s",
					DISPLAY_NAMES[ip->display_mode],
					CONVOLUTION_NAMES[ip->convolution_mode],
					NORMALIZATION_NAMES[ip->normalization_mode]);

	SDL_SetWindowTitle(ip->display->window, buf);
}

void ImageProcessor_load( struct ImageProcessor* ip, char* filename ) {
	ip->edge_detector->image = IMG_Load(filename);
	if(!ip->edge_detector->image) {
		die(ip, IMG_GetError());
	}
	
	SDL_Surface* image = ip->edge_detector->image;
	ip->edge_detector->edges = SDL_CreateRGBSurface( 0,
																									 image->w,
																									 image->h,
																									 32,
																									 image->format->Rmask,
																									 image->format->Gmask,
																									 image->format->Bmask,
																									 image->format->Amask	); 
	if(!ip->edge_detector->edges) {
		die(ip, SDL_GetError());
	}

	if( SDL_CreateWindowAndRenderer(image->w, image->h, 0,
																	&ip->display->window, &ip->display->renderer) < 0 ) {
		die(ip, SDL_GetError());
	}
	
	ImageProcessor_set_display_mode( ip, SHOW_EDGES );
	ImageProcessor_set_convolution_mode( ip, SOBEL );
	ImageProcessor_set_normalization_mode( ip, LOCAL );
}

void ImageProcessor_detect( struct ImageProcessor* ip ) {

	if( ip->normalization_mode == LOCAL ) {
		EdgeDetector_detect_normalize_local(ip->edge_detector);
	} else if (ip->normalization_mode == GLOBAL ) {
		EdgeDetector_detect_normalize_global(ip->edge_detector);
	} else if (ip->normalization_mode == NONE ) {
		EdgeDetector_detect_normalize_none(ip->edge_detector);
	}
	
}

void ImageProcessor_show( struct ImageProcessor* ip ) {
	SDL_Surface *img;
	
	switch(ip->display_mode) {
	case SHOW_EDGES:
		img = ip->edge_detector->edges;
		break;
	case SHOW_ORIGINAL:
		img = ip->edge_detector->image;
		break;
	default:
		return;
	}

	SDL_Texture *texture = SDL_CreateTextureFromSurface(ip->display->renderer, img);

	SDL_RenderClear(ip->display->renderer); 
	SDL_RenderCopy(ip->display->renderer, texture, NULL, NULL);
	SDL_RenderPresent(ip->display->renderer);
	
	SDL_DestroyTexture(texture);
}

void ImageProcessor_destroy( struct ImageProcessor* ip ) {
	if(ip) {
		/* destroy the struct that identifies edges in image */
		EdgeDetector_destroy(ip->edge_detector);

		/* tear down the window and rendering structs */
		Display_destroy( ip->display );
		
		free(ip);
	}
}

struct EdgeDetector* EdgeDetector_create( void ) {
	struct EdgeDetector* ed = malloc(sizeof( struct EdgeDetector ));

	if(ed) {
		ed->edges = NULL;
		ed->image = NULL;
		SobelConvolutionMatrix_init(&ed->matrix);
	}

	return ed;
}

void EdgeDetector_detect_normalize_local( struct EdgeDetector* ed ) {
	int i;
	Uint32 pixel, max, min;
	Uint32* gradients;

	max = 0; min = 0xFFFFFFFF;
	gradients = malloc( sizeof(Uint32) * ed->image->w * ed->image->h );

	if(!gradients) {
		printf("Unable to allocate memory for computing gradients!\n");
		return;
	}
	
	/* clear the edge image */
	SDL_FillRect(ed->edges, NULL, 0x000000);

	/* compute the gradient for each point on the image */
	for( i=0; i<ed->image->h*ed->image->w; i++ ) {
		gradients[i] = ConvolutionMatrix_convolve( &ed->matrix, ed->image, i%ed->image->w, i/ed->image->w);
		if(gradients[i] > max ) {
			max = gradients[i];			
		}
		if( gradients[i] < min ) {
			min = gradients[i];
		}
	}

	/* normalize values to be between 0 and 255. Strictly speaking, I
	 * shouldn't do this, but I'm not writing an image processing
	 * library. I'm doing this for demonstration purposes and
	 * normalisation just makes it easier to see the final image. Could
	 * also normalise according to the maximum possible output of the
	 * convolution matrix. This would be more correct and would not
	 * require the use of malloc above */
	for( i=0; i<ed->image->h*ed->image->w; i++ ) {
		gradients[i] = ((double)(gradients[i] - min))/(max-min) * 255;
	}

	/* write values back to the image */
	for( i=0; i<ed->image->h*ed->image->w; i++ ) {
		pixel = compress(ed->edges->format, gradients[i], gradients[i], gradients[i], 0xFF);
		set_pixel(ed->edges, i%ed->edges->w, i/ed->edges->w, pixel);
	}
	
	free(gradients);
}

void EdgeDetector_detect_normalize_global( struct EdgeDetector* ed ) {
	int x,y;
	Uint32 pixel, max_output;
	Uint32 gradient;
	
	/* clear the edge image */
	SDL_FillRect(ed->edges, NULL, 0x000000);

	max_output = ConvolutionMatrix_max_output( &ed->matrix, 255 );
	
	/* compute the gradient for each point on the image */
	for( y=0; y<ed->image->h; y++ ) {
		for( x=0; x<ed->image->w; x++ ) {
			gradient = ConvolutionMatrix_convolve( &ed->matrix, ed->image, x, y);
			gradient = (((double)gradient)/max_output) * 255;
			pixel = compress(ed->edges->format, gradient, gradient, gradient, 0xFF);
			set_pixel(ed->edges, x, y, pixel);
		}
	}
}

void EdgeDetector_detect_normalize_none( struct EdgeDetector* ed ) {
	int x,y;
	Uint32 pixel;
	Uint32 gradient;
	
	/* clear the edge image */
	SDL_FillRect(ed->edges, NULL, 0x000000);
	
	/* compute the gradient for each point on the image */
	for( y=0; y<ed->image->h; y++ ) {
		for( x=0; x<ed->image->w; x++ ) {
			gradient = ConvolutionMatrix_convolve( &ed->matrix, ed->image, x, y);
			pixel = compress(ed->edges->format, gradient, gradient, gradient, 0xFF);
			set_pixel(ed->edges, x, y, pixel);
		}
	}
}

void EdgeDetector_destroy( struct EdgeDetector* ed ) {
	if(ed) {
		/* don't need to check if image or edges are null. SDL will do
		 * that for us */
		SDL_FreeSurface(ed->image);
		SDL_FreeSurface(ed->edges);
		free(ed);
	}
}

struct Display* Display_create( void ) {
	struct Display* d = malloc(sizeof(struct Display));
	if(d) {
		d->window = NULL;
		d->renderer = NULL;
	}
	return d;
}

void Display_destroy( struct Display *d ) {
	if(d) {
		/* SDL does handle these NULL checks, but I've noticed that
		 * SDL_DestroyRenderer and SDL_DestroyWindow cause memory leaks,
		 * so I'll only call them if I need to. */
		if(d->renderer) {
			SDL_DestroyRenderer(d->renderer);
		}
		if(d->window) {
			SDL_DestroyWindow(d->window);
		}
			
		free(d);
	}
}



void SobelConvolutionMatrix_init( struct ConvolutionMatrix* matrix ) {
	struct ConvolutionMatrix m = {
		.width = SOBEL_MATRIX_WIDTH,
		.height = SOBEL_MATRIX_HEIGHT,
		.x_kernel = {
			{1, 0, -1},
			{2, 0, -2},
			{1, 0, -1}
		},
		.y_kernel = {
			{ 1,  2,  1},
			{ 0,  0,  0},
			{-1, -2, -1}
		}
	};

	*matrix = m;
}

void RobertsConvolutionMatrix_init( struct ConvolutionMatrix* matrix ) {
	struct ConvolutionMatrix m = {
		.width = ROBERTS_MATRIX_WIDTH,
		.height = ROBERTS_MATRIX_HEIGHT,
		.x_kernel = {
			{ 1,  0 },
			{ 0, -1 }
		},
		.y_kernel = {
			{ 0,  1 },
			{-1,  0 }
		}
	};

	*matrix = m;
}

void PrewittConvolutionMatrix_init( struct ConvolutionMatrix* matrix ) {
	struct ConvolutionMatrix m = {
		.width = PREWITT_MATRIX_WIDTH,
		.height = PREWITT_MATRIX_HEIGHT,
		.x_kernel = {
			{1, 0, -1},
			{1, 0, -1},
			{1, 0, -1}
		},
		.y_kernel = {
			{ 1,  1,  1},
			{ 0,  0,  0},
			{-1, -1, -1}
		}
	};

	*matrix = m;
}

void ScharrConvolutionMatrix_init( struct ConvolutionMatrix* matrix ) {
	struct ConvolutionMatrix m = {
		.width = SCHARR_MATRIX_WIDTH,
		.height = SCHARR_MATRIX_HEIGHT,
		.x_kernel = {
			{ 3, 0,  -3},
			{10, 0, -10},
			{ 3, 0,  -3}
		},
		.y_kernel = {
			{ 3, 10,  3},
			{ 0,  0,  0},
			{-3, -10, -3}
		}
	};

	*matrix = m;
}

Uint32 ConvolutionMatrix_convolve ( struct ConvolutionMatrix* matrix, SDL_Surface* image, int x0, int y0 ) {
	int x, y;
	int grad_x, grad_y;
	Uint32 pixel, r, g, b, a;

	int min_y = y0-matrix->height/3;
	int min_x = x0-matrix->width/3;

	grad_x = 0;
	grad_y = 0;
	
	for( y=min_y; y<min_y+matrix->height; y++ ) {
		for( x=min_x; x<min_x+matrix->width; x++ ) {
			if( x>=0 && x<=image->w && y>=0 && y<image->h) {
				pixel = get_pixel(image, x, y );
				explode(image->format, pixel, &r, &g, &b, &a );
				pixel = to_greyscale( image->format, r, g, b, a );
			
				grad_x += matrix->x_kernel[y-min_y][x-min_x]*pixel;
				grad_y += matrix->y_kernel[y-min_y][x-min_x]*pixel;
			}
		}
	}

	return sqrt(grad_x*grad_x + grad_y*grad_y);
}

Uint32 ConvolutionMatrix_max_output ( struct ConvolutionMatrix* matrix, Uint32 max_input ) {
	int x, y;
	int grad_x, grad_y;
	int pos_grad_x, pos_grad_y;
	int neg_grad_x, neg_grad_y;

	pos_grad_x = 0;
	pos_grad_y = 0;
	neg_grad_x = 0;
	neg_grad_y = 0;
	
	for( y=0; y<matrix->height; y++ ) {
		for( x=0; x<matrix->width; x++ ) {
			if( matrix->x_kernel[y][x] > 0 ) {
				pos_grad_x += max_input*matrix->x_kernel[y][x];
			} else {
				neg_grad_x += max_input*abs(matrix->x_kernel[y][x]);
			}
			if( matrix->y_kernel[y][x] > 0 ) {
				pos_grad_y += max_input*matrix->y_kernel[y][x];
			} else {
				neg_grad_y += max_input*abs(matrix->y_kernel[y][x]);
			}
		}
	}

	grad_x = (pos_grad_x > neg_grad_x)? pos_grad_x : neg_grad_x;
	grad_y = (pos_grad_y > neg_grad_y)? pos_grad_y : neg_grad_y;

	return sqrt(grad_x*grad_x + grad_y*grad_y);
}
