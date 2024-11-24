#include "ewglu.h"


#define PIXELSIZE 4
#define PBUFFER_STRIDE_ELEMCOUNT 5
#define PBUFFER_MAX_MULT 1 // multiply the amount of pixels reserved?

#define FLG_CLEARBUFFER 1
#define FLG_NOMOUSEPOS  2


// NOTE: RGB's are from 0.0 to 1.0
//


/*

   TODO. (not in any order)
      
     * ray casting.


    - move coordinate mapping to shaders,
    
    
    - glow/light
        - affect:
            0. doesnt affect neighbor pixels
            1. affects X radius neighbor pixels

    - function to return pixels nearby?

    - textures

    - animations

    - pixel blending (lerp?)

    - 2D matrixes?



*/

struct sandbox_t {
    GLFWwindow* win;
    int running;

    int max_col;
    int max_row;
    int center_row;
    int center_col;

    int win_width;
    int win_height;

    unsigned int vbo;
    unsigned int vao;
    unsigned int defshdr; // default shader

    float* buffer;
    size_t bufsize;
    size_t num_pixels;    // number of pixels needed to render after calling 'loop_callback'.
    size_t num_maxpixels; // number of max pixels can be drawn.

    double time;
    double dt;  // delta time.
    double mouse_x;
    double mouse_y;
    int mouse_scroll;

    // buffer for raycast. 
    // to use it first call allocate_rcbuf() to initialize it.
    // it will be allocated (num_maxpixels * sizeof *rcbuf) bytes of memory
    // and rcbuf_avail is set to 1 if success.
    // call free_rcbuf after use, 
    // it is also called when run_sandbox exits the loop
    // you can set the ID with 'rcbuf_setid(sanbox_t*, x, y, id)' 
    // zero is treated as 'air'
    // and then the function 'raycast(sandbox_t*, start_x, start_y, end_x, end_y)'
    // will return the ID if it hit anything non zero.
    int* rcbuf;
    int rcbuf_avail;


    int flags;
};

#define _PI   3.1415
#define _2PI  6.2831
#define _PI_R 0.0174

#define GLSL_VERSION "#version 430\n"

static const char SANDBOX_VERTEX_SRC[] =  // TODO: map coordinates in shader
GLSL_VERSION
"layout(location = 0) in vec2 i_pos;"
"layout(location = 1) in vec3 i_color;"
"out vec3 f_color;"

"void main() {"
    "gl_Position = vec4(i_pos.x, i_pos.y, 0.0, 1.0);"
    "f_color = i_color;"
"}"
;

static const char SANDBOX_FRAGMENT_SRC[] = 
GLSL_VERSION
"in vec3 f_color;"
"out vec4 out_color;"

"void main() {"
    "out_color = vec4(f_color.xyz, 1.0);"
"}"
;

int init_sandbox(struct sandbox_t* sbox, int width, int height, const char* window_name);
void run_sandbox(struct sandbox_t* sbox,
        void(*loop_callback)(struct sandbox_t* sbox, void*), void* userptr);
void free_sandbox(struct sandbox_t* sbox);

//  UTILITY FUNCTIONS ----

void  rainbow_palette(float t, float* r, float* g, float* b);
float vdistance(float x0, float y0, float x1, float y1);
float vdot(float x0, float y0,  float x1, float y1);
float vlength(float x, float y);
float vangle(float x0, float y0, float x1, float y1);
size_t getindexp(struct sandbox_t* sbox, int x, int y);

//  RAYCAST FUNCTIONS ----

#define RAYCAST_AIRID 0

int allocate_rcbuf(struct sandbox_t* sbox);
void free_rcbuf(struct sandbox_t* sbox);

void rcbuf_setid(struct sandbox_t* sbox, int x, int y, int id);
int  raycast(struct sandbox_t* sbox, 
        int start_x, int start_y,
        int end_x,  int end_y,
        int* hit_x, int* hit_y);



//  "DRAWING" FUNCTIONS ----

void setpixel(struct sandbox_t* sbox, 
        float x, float y,
        float r, float g, float b);

void fillcircle(struct sandbox_t* sbox, 
        float fx, float fy, float radius,
        float r, float g, float b);

void setline(struct sandbox_t* sbox, 
        int x0, int y0, int x1, int y1,
        float r, float g, float b);









