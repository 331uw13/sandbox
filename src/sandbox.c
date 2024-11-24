#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "sandbox.h"

void run_sandbox(struct sandbox_t* sbox,
        void(*loop_callback)(struct sandbox_t* sbox, void*), void* userptr) {
    
    int isbufferleft_mapped = 0;

    while(!glfwWindowShouldClose(sbox->win) && sbox->running) {
        const double t_framestart = glfwGetTime();
        
        glClear(GL_COLOR_BUFFER_BIT);

        glBindBuffer(GL_ARRAY_BUFFER, sbox->vbo);
        sbox->buffer = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        if(!sbox->buffer) {
            fprintf(stderr, "glMapBuffer returned NULL!\n");
            break;
        }
        isbufferleft_mapped = 1;

        if(!(sbox->flags & FLG_NOMOUSEPOS)) {
            glfwGetCursorPos(sbox->win, &sbox->mouse_x, &sbox->mouse_y);
            if(sbox->mouse_x > 0.0) {
                sbox->mouse_x /= PIXELSIZE;
            }
            if(sbox->mouse_y > 0.0) {
                sbox->mouse_y /= PIXELSIZE;
            }
        }

        sbox->time = glfwGetTime();


        if(loop_callback && sbox->buffer) {
            if((sbox->flags & FLG_CLEARBUFFER)) {
                memset(sbox->buffer, -1, sbox->bufsize);
            }
            loop_callback(sbox, userptr);

            glUnmapNamedBuffer(sbox->vbo);
            glBindVertexArray(sbox->vao);

            glUseProgram(sbox->defshdr);
            glDrawArrays(GL_POINTS, 0, sbox->num_maxpixels);

            sbox->buffer = NULL;
            sbox->num_pixels = 0;
            isbufferleft_mapped = 0;

        }


        sbox->mouse_scroll = 0;

        glfwSwapBuffers(sbox->win);
        
        sbox->dt = glfwGetTime() - t_framestart;
        glfwPollEvents();
    }

    if(isbufferleft_mapped) {
        glUnmapNamedBuffer(sbox->vbo);
        
        int ismapped = 0;
        glGetBufferParameteriv(GL_BUFFER_MAPPED, sbox->vbo, &ismapped);
        
        if(ismapped) {
            printf("buffer was left mapped, it is unmapped now.\n");
        }
    }
}

void free_sandbox(struct sandbox_t* sbox) {
    glDeleteVertexArrays(1, &sbox->vao);
    glDeleteBuffers(1, &sbox->vbo);

    ewglu_delete_program(sbox->defshdr);
    free_rcbuf(sbox);

    glfwDestroyWindow(sbox->win);
    glfwTerminate();
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    struct sandbox_t* sbox = (struct sandbox_t*) glfwGetWindowUserPointer(window);
    if(sbox) {
        sbox->mouse_scroll = (int)yoffset;
    }
}

int init_sandbox(struct sandbox_t* sbox, int width, int height, const char* window_name) {
    int result = 0;
    //struct sandbox_t* sbox = NULL;
    //sbox = malloc(sizeof *sbox);


    sbox->running = 0;
    sbox->buffer = NULL;
    sbox->time = 0.0;
    sbox->dt = 0.0;
    sbox->mouse_x = 0.0;
    sbox->mouse_y = 0.0;
    sbox->flags = 0;

    if(!ewglu_init_glfw()) {
        goto error;
    }

    glfwWindowHint(GLFW_RESIZABLE, 0);
    sbox->win = NULL;
    sbox->win = ewglu_init(width, height, window_name, NULL, 4, 3);
    if(!sbox->win) {
        goto error;
    }

    glfwSetWindowUserPointer(sbox->win, sbox);
    glfwSetScrollCallback(sbox->win, scroll_callback);

    glfwGetWindowSize(sbox->win, &sbox->win_width, &sbox->win_height);
    
    if(sbox->win_width * sbox->win_height <= 0) {
        fprintf(stderr, "Window size seems to be way too small\n");
        goto error;
    }

    
    const size_t stridesize = sizeof(float) * PBUFFER_STRIDE_ELEMCOUNT;
    sbox->max_col = sbox->win_width / PIXELSIZE;
    sbox->max_row = sbox->win_height / PIXELSIZE;
    sbox->num_maxpixels = (sbox->max_col * sbox->max_row) * PBUFFER_MAX_MULT;
    sbox->bufsize = stridesize * sbox->num_maxpixels;

    sbox->center_col = sbox->max_col / 2;
    sbox->center_row = sbox->max_row / 2;

    printf("%ix%i | %ix%i\n", 
            sbox->win_width, sbox->win_height,
            sbox->max_col, sbox->max_row);

    printf("buffer size: | %li bytes / %li kB |\n", 
            sbox->bufsize,
            sbox->bufsize / 1000);

    sbox->vao = 0;
    sbox->vbo = 0;

    glGenVertexArrays(1, &sbox->vao);
    glBindVertexArray(sbox->vao);

    glGenBuffers(1, &sbox->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, sbox->vbo);
    glBufferData(GL_ARRAY_BUFFER, sbox->bufsize, NULL, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stridesize, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stridesize, (void*)(sizeof(float)*2));
    glEnableVertexAttribArray(1);

    sbox->defshdr = ewglu_create_program
                    (SANDBOX_VERTEX_SRC, SANDBOX_FRAGMENT_SRC);

    glPointSize(PIXELSIZE);

    sbox->rcbuf = NULL;
    sbox->rcbuf_avail = 0,

    sbox->running = 1;
    sbox->flags |= FLG_CLEARBUFFER;


    result = 1;
error:
    return result;
}


// UTILITY FUNCTIONS

void rainbow_palette(float t, float* r, float* g, float* b) {
    // for more information about this check out:
    // https://iquilezles.org/articles/palettes/
    *r = 0.5+0.5 * cos(_2PI * t);
    *g = 0.5+0.5 * cos(_2PI * (t+0.33));
    *b = 0.5+0.5 * cos(_2PI * (t+0.67));
}

float vdistance(float x0, float y0, float x1, float y1) {
    float dx = x0 - x1;
    float dy = y0 - y1;
    return sqrt(dx*dx + dy*dy);
}

float vdot(float x0, float y0,  float x1, float y1) {
    return (x0 * x1) + (y0 * y1);
}

float vlength(float x, float y) {
    return sqrt(vdot(x, y, x, y));
}

float vangle(float x0, float y0, float x1, float y1) {
    float delx = x1 - x0;
    float dely = y1 - y0;
    return (atan2(dely, delx)*180.0)/_PI;
}

size_t getindexp(struct sandbox_t* sbox, int x, int y) {
    if(x < 0) { x = 0; }
    if(y < 0) { y = 0; }

    size_t index = y * sbox->max_col + x;

    if(index >= sbox->num_maxpixels) {
        index = sbox->num_maxpixels-1;
    }


    return index;
}



// RAYCAST FUNCTIONS

int allocate_rcbuf(struct sandbox_t* sbox) {
    int res = 0;

    if(sbox->rcbuf != NULL) {
        fprintf(stderr, "raycast buffer is already initialized ???\n");
        goto error;
    }

    size_t rcbufsize = sbox->num_maxpixels * sizeof *sbox->rcbuf;
    sbox->rcbuf = malloc(rcbufsize);

    if(!sbox->rcbuf) {
        fprintf(stderr, "%s | failed to allocate memory for raycast buffer %li bytes of memory\n",
                __func__, rcbufsize);
        goto error;
    }

    memset(sbox->rcbuf, 0, rcbufsize);

    sbox->rcbuf_avail = 1;
    res = 1;

error:
    return res;
}

void free_rcbuf(struct sandbox_t* sbox) {
    if(sbox->rcbuf) {
        free(sbox->rcbuf);
    }
    sbox->rcbuf_avail = 0;
}

void rcbuf_setid(struct sandbox_t* sbox, int x, int y, int id) {
    if(sbox->rcbuf && sbox->rcbuf_avail) {
        sbox->rcbuf[getindexp(sbox, x, y)] = id;
    }
}

int raycast(struct sandbox_t* sbox, 
        int start_x, int start_y,
        int end_x,  int end_y,
        int* hit_x, int* hit_y)
{
    int id = 0;

    int width = end_x - start_x;
    int height = end_y - start_y;
    int dx0 = 0;
    int dy0 = 0;
    int dx1 = 0;
    int dy1 = 0;

    dx1 = dx0 = (width < 0) ? -1 : 1;
    dy0 = (height < 0) ? -1 : 1;

    int aw = abs(width);
    int ah = abs(height);
    int longest = aw;
    int shortest = ah;

    if(longest < shortest) {
        longest = ah;
        shortest = aw;
        dy1 = (height < 0) ? -1 : 1;
        dx1 = 0;
    }

    int numerator = longest >> 1;

    for(int i = 0; i < longest; i++) {

        if((id = sbox->rcbuf[getindexp(sbox, start_x, start_y)])
                != RAYCAST_AIRID) {

            if(hit_x) {
                *hit_x = start_x;
            }
            if(hit_y) {
                *hit_y = start_y;
            }

            break;
        }

        numerator += shortest;
        if(numerator > longest) {
            numerator -= longest;
            start_x += dx0;
            start_y += dy0;
        }
        else {
            start_x += dx1;
            start_y += dy1;
        }
    }

    return id;
}


// "DRAWING" FUNCTIONS

void setpixel(struct sandbox_t* sbox, float x, float y,
        float r, float g, float b)
{
    x = floorf(x);
    y = floorf(y);

    size_t indx = getindexp(sbox, (int)x, (int)y);

    if((sbox->num_pixels+1 <= sbox->num_maxpixels)
    && sbox->buffer) {

        indx *= PBUFFER_STRIDE_ELEMCOUNT; 

        sbox->buffer[indx]   = map(x, 0, sbox->max_col, -1.0,  1.0);
        sbox->buffer[indx+1] = map(y, 0, sbox->max_row,  1.0, -1.0);
        sbox->buffer[indx+2] = r;
        sbox->buffer[indx+3] = g;
        sbox->buffer[indx+4] = b;

        sbox->num_pixels++;
    }

}

void fillcircle(struct sandbox_t* sbox, 
        float fx, float fy, float radius,
        float r, float g, float b)
{

    int ystart = fy - radius;
    int yend   = fy + radius;
    
    int xstart = fx - radius;
    int xend   = fx + radius;
    

    for(int y = ystart; y <= yend; y++) {
        for(int x = xstart; x <= xend; x++) {
    
            float dst = vdistance(x+0.5, y+0.5, fx, fy);

            if(dst <= radius) {
                setpixel(sbox,x, y,  r, g, b);
            }
        }
    }
}

void setline(struct sandbox_t* sbox, 
        int x0, int y0, int x1, int y1,
        float r, float g, float b)
{
    int width = x1-x0;
    int height = y1-y0;
    int dx0 = 0;
    int dy0 = 0;
    int dx1 = 0;
    int dy1 = 0;

    dx1 = dx0 = (width < 0) ? -1 : 1;
    dy0 = (height < 0) ? -1 : 1;

    int aw = abs(width);
    int ah = abs(height);
    int longest = aw;
    int shortest = ah;

    if(longest < shortest) {
        longest = ah;
        shortest = aw;
        dy1 = (height < 0) ? -1 : 1;
        dx1 = 0;
    }

    int numerator = longest >> 1;

    for(int i = 0; i < longest; i++) {
        
        setpixel(sbox, x0, y0, r,g,b);

        numerator += shortest;
        if(numerator > longest) {
            numerator -= longest;
            x0 += dx0;
            y0 += dy0;
        }
        else {
            x0 += dx1;
            y0 += dy1;
        }
    }
}


