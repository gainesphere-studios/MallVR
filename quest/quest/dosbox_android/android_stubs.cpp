/* Android build of DOSBox-X: no-op stand-ins for desktop-only parts that the rest of the code still calls.
   Voodoo OpenGL path (needs desktop GL; the software Voodoo stays), host serial ports, RetroWave OPL hardware. */
struct _voodoo_state; struct _poly_extra_data; struct _COMPORT;

bool Voodoo_OGL_Active() { return false; }
bool voodoo_ogl_init(_voodoo_state *) { return false; }
void voodoo_ogl_shutdown(_voodoo_state *) {}
void voodoo_ogl_set_window(_voodoo_state *) {}
void voodoo_ogl_swap_buffer(void) {}
void voodoo_ogl_vblank_flush(void) {}
void voodoo_ogl_clear(void) {}
void voodoo_ogl_fastfill(void) {}
void voodoo_ogl_clip_window(_voodoo_state *) {}
void voodoo_ogl_texture_clear(unsigned int, int) {}
void voodoo_ogl_invalidate_paltex(void) {}
void voodoo_ogl_draw_pixel(int, int, bool, bool, int, int, int, int) {}
void voodoo_ogl_draw_pixel_pipeline(int, int, int, int, int) {}
unsigned int voodoo_ogl_read_pixel(int, int) { return 0; }
void voodoo_ogl_draw_triangle(_poly_extra_data *) {}

bool SERIAL_open(const char *, _COMPORT **) { return false; }
void SERIAL_close(_COMPORT *) {}
bool SERIAL_setCommParameters(_COMPORT *, int, char, int, int) { return false; }
bool SERIAL_sendchar(_COMPORT *, char) { return false; }

extern "C" int retrowave_init_linux_spi(void *, const char *, int, int) { return -1; }
extern "C" int retrowave_init_posix_serialport(void *, const char *) { return -1; }
