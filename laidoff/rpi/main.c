#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "lwgl.h"
#include "laidoff.h"
#include "lwmacro.h"
#include "lwlog.h"

#ifndef BOOL
#define BOOL int
#endif

// Working directory change
#if LW_PLATFORM_OSX || LW_PLATFORM_RPI
#include <dirent.h>
#include <unistd.h>
#endif

#if LW_PLATFORM_WIN32
#define LwChangeDirectory(x) SetCurrentDirectory(x)
#else
#define LwChangeDirectory(x) chdir(x)
#endif

#define INITIAL_SCREEN_RESOLUTION_X 640 //(1920)
#define INITIAL_SCREEN_RESOLUTION_Y 360 //(1080)

typedef struct
{
   uint32_t screen_width;
   uint32_t screen_height;
// OpenGL|ES objects
   DISPMANX_DISPLAY_HANDLE_T dispman_display;
   DISPMANX_ELEMENT_HANDLE_T dispman_element;
   EGLDisplay display;
   EGLSurface surface;
   EGLContext context;
   GLuint tex[6];
// model rotation vector and direction
   GLfloat rot_angle_x_inc;
   GLfloat rot_angle_y_inc;
   GLfloat rot_angle_z_inc;
// current model rotation angles
   GLfloat rot_angle_x;
   GLfloat rot_angle_y;
   GLfloat rot_angle_z;
// current distance from camera
   GLfloat distance;
   GLfloat distance_inc;
// pointers to texture buffers
   char *tex_buf1;
   char *tex_buf2;
   char *tex_buf3;
} CUBE_STATE_T;

static CUBE_STATE_T _state, *state=&_state;

static const char *const evval[3] = {
    "RELEASED",
    "PRESSED ",
    "REPEATED"
};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_pos_callback(GLFWwindow* window, double x, double y);
void destroy_ext_sound_lib();

static void error_callback(int error, const char* description)
{
	LOGE("Error: %s\n", description);
}

static BOOL directory_exists(const char* szPath)
{
#if LW_PLATFORM_WIN32
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    DIR* dir = opendir(szPath);
    if (dir)
    {
        return 1;
    }
    else
    {
        return 0;
    }
#endif
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
}

void print_welcome_message() {
	LOGI(LWU("용사는 휴직중 LAID OFF v0.1\n"));
}

void change_working_directory() {
	while (!directory_exists("assets") && LwChangeDirectory(".."))
	{
	}
}

static void init_ogl(CUBE_STATE_T *state)
{
   int32_t success = 0;
   EGLBoolean result;
   EGLint num_config;

   static EGL_DISPMANX_WINDOW_T nativewindow;

   DISPMANX_UPDATE_HANDLE_T dispman_update;
   VC_RECT_T dst_rect;
   VC_RECT_T src_rect;

   static const EGLint attribute_list[] =
   {
	  EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_RED_SIZE, 5,
      EGL_GREEN_SIZE, 6,
      EGL_BLUE_SIZE, 5,
      EGL_ALPHA_SIZE, 0,
	  EGL_DEPTH_SIZE, 8,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_NONE
   };
   
   EGLConfig config;

   // get an EGL display connection
   state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   assert(state->display!=EGL_NO_DISPLAY);

   // initialize the EGL display connection
   result = eglInitialize(state->display, NULL, NULL);
   assert(EGL_FALSE != result);

   // get an appropriate EGL frame buffer configuration
   result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
   assert(EGL_FALSE != result);

   const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION,
                                      2,  // Request OpenGL ES2.0
                                      EGL_NONE};
   // create an EGL rendering context
   state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attribs);
   assert(state->context!=EGL_NO_CONTEXT);

   // create an EGL window surface
   success = graphics_get_display_size(0 /* LCD */, &state->screen_width, &state->screen_height);
   assert( success >= 0 );
   
   //state->screen_width = 320;
   //state->screen_height = 180;

   dst_rect.x = 0;//800;
   dst_rect.y = 0;//300;
   dst_rect.width = state->screen_width;
   dst_rect.height = state->screen_height;
      
   src_rect.x = 0;
   src_rect.y = 0;
   src_rect.width = state->screen_width << 16;
   src_rect.height = state->screen_height << 16;        

   state->dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
   dispman_update = vc_dispmanx_update_start( 0 );

   VC_DISPMANX_ALPHA_T         dispman_alpha;

	dispman_alpha.flags = DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS; 
	dispman_alpha.opacity = 0xFF; 
	dispman_alpha.mask = NULL; 

   state->dispman_element = vc_dispmanx_element_add ( dispman_update, state->dispman_display,
      0/*layer*/, &dst_rect, 0/*src*/,
      &src_rect, DISPMANX_PROTECTION_NONE, &dispman_alpha, 0/*clamp*/, 0/*transform*/);
      
   nativewindow.element = state->dispman_element;
   nativewindow.width = state->screen_width;
   nativewindow.height = state->screen_height;
   vc_dispmanx_update_submit_sync( dispman_update );
      
   state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
   assert(state->surface != EGL_NO_SURFACE);

   // connect the context to the surface
   result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
   assert(EGL_FALSE != result);
}

void init_window_and_gl_context() {
	// Clear application state
	memset( state, 0, sizeof( *state ) );
      
	// Start OGLES
	init_ogl(state);
	
	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("Current OpenGL Version: %s\n", version);
}

void randomize_random_seed() {
	srand((unsigned int)time(0));
}

int main(void)
{
	print_welcome_message();
	
	change_working_directory();
	
	// Required for RPI
	bcm_host_init();

	init_window_and_gl_context();

	randomize_random_seed();

	LWCONTEXT* pLwc = lw_init();

	int width = state->screen_width;
	int height = state->screen_height;
	lw_set_size(pLwc, width, height);
	
	const char *dev = "/dev/input/by-id/usb-_USB_Keyboard-event-kbd";
    struct input_event ev;
    ssize_t n;
    int fd;

    fd = open(dev, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
        return EXIT_FAILURE;
    }
	
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	
	struct timeval t1, t2;
	struct timezone tz;
	float deltatime;
	float totaltime = 0.0f;
	unsigned int frames = 0;

	gettimeofday(&t1, &tz);

	lwc_start_logic_thread(pLwc);

	while (!pLwc->quit_request)
	{
		gettimeofday(&t2, &tz);
		deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
		t1 = t2;
		
		n = read(fd, &ev, sizeof ev);
        if (n == (ssize_t)-1) {
            
        } else
        if (n != sizeof ev) {
            
        }
		
		if (ev.type == EV_KEY && ev.value >= 0 && ev.value <= 2) {
            LOGI("%s 0x%04x (%d)\n", evval[ev.value], (int)ev.code, (int)ev.code);
			
			key_callback((GLFWwindow*)pLwc, (int)ev.code, 0, 0, 0);
		}

		/*
		lwcontext_set_safe_to_start_render(pLwc, 0);

		lwc_update(pLwc, 1.0/60);

		lwcontext_set_safe_to_start_render(pLwc, 1);
		*/

		lwc_render(pLwc);

		eglSwapBuffers(state->display, state->surface);
		
		totaltime += deltatime;

		frames++;

		if (totaltime > 2.0f)
		{
			printf("%4d frames rendered in %1.4f seconds. -> FPS=%3.4f\n", frames, totaltime, frames/totaltime);
			totaltime -= 2.0f;
			frames = 0;
		}
	}

	destroy_ext_sound_lib();

	lw_on_destroy(pLwc);
	
	fflush(stdout);
    
    printf(LWU("용사는 휴직중 종료\n"));

	exit(EXIT_SUCCESS);
}

void lw_app_quit(LWCONTEXT* pLwc)
{
	pLwc->quit_request = 1;
}

int request_get_today_played_count()
{
	return 0;
}

int request_get_today_playing_limit_count()
{
	return 5;
}

void request_on_game_start()
{
	// do nothing
}

void request_on_game_over(int point)
{
	// do nothing
}

void request_boast(int point)
{
	// do nothing
}

int request_is_retryable()
{
	return 1;
}

int request_is_boasted()
{
	return 0;
}

int request_get_highscore()
{
	return 0;
}

void request_set_highscore(int highscore)
{
	// do nothing
}
