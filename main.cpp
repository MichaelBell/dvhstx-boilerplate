#include <stdio.h>
#include "hardware/uart.h"
#include "drivers/dvhstx/dvhstx.hpp"
#include "libraries/pico_graphics/pico_graphics_dvhstx.hpp"

using namespace pimoroni;

#define FRAME_WIDTH 320
#define FRAME_HEIGHT 180

#define CENTRE_X (FRAME_WIDTH / 2)
#define CENTRE_Y (FRAME_HEIGHT / 2)

  // 3x3 matrix for coordinate transformations
  struct mat3_t {
    float v00 = 0.0f, v10 = 0.0f, v20 = 0.0f, v01 = 0.0f, v11 = 0.0f, v21 = 0.0f, v02 = 0.0f, v12 = 0.0f, v22 = 0.0f;
    mat3_t() = default;
    mat3_t(const mat3_t &m) = default;
    inline mat3_t& operator*= (const mat3_t &m) {        
      float r00 = this->v00 * m.v00 + this->v01 * m.v10 + this->v02 * m.v20;
      float r01 = this->v00 * m.v01 + this->v01 * m.v11 + this->v02 * m.v21;
      float r02 = this->v00 * m.v02 + this->v01 * m.v12 + this->v02 * m.v22;
      float r10 = this->v10 * m.v00 + this->v11 * m.v10 + this->v12 * m.v20;
      float r11 = this->v10 * m.v01 + this->v11 * m.v11 + this->v12 * m.v21;
      float r12 = this->v10 * m.v02 + this->v11 * m.v12 + this->v12 * m.v22;
      float r20 = this->v20 * m.v00 + this->v21 * m.v10 + this->v22 * m.v20;
      float r21 = this->v20 * m.v01 + this->v21 * m.v11 + this->v22 * m.v21;
      float r22 = this->v20 * m.v02 + this->v21 * m.v12 + this->v22 * m.v22;    
      this->v00 = r00; this->v01 = r01; this->v02 = r02;
      this->v10 = r10; this->v11 = r11; this->v12 = r12;
      this->v20 = r20; this->v21 = r21; this->v22 = r22;
      return *this;
    }

    static mat3_t identity() {mat3_t m; m.v00 = m.v11 = m.v22 = 1.0f; return m;}
    static mat3_t rotationxy(float a) {
      float c = cosf(a), s = sinf(a); mat3_t r = mat3_t::identity();
      r.v00 = c; r.v01 = -s; r.v10 = s; r.v11 = c; return r;}
    static mat3_t rotationyz(float a) {
      float c = cosf(a), s = sinf(a); mat3_t r = mat3_t::identity();
      r.v11 = c; r.v12 = -s; r.v21 = s; r.v22 = c; return r;}
    static mat3_t rotationxz(float a) {
      float c = cosf(a), s = sinf(a); mat3_t r = mat3_t::identity();
      r.v00 = c; r.v02 = -s; r.v20 = s; r.v22 = c; return r;}
    static mat3_t scale(float x, float y, float z) {
      mat3_t r = mat3_t::identity(); r.v00 = x; r.v11 = y; r.v22 = z; return r;}
  };

  struct vec3_t {
    float x = 0.f, y = 0.f, z = 0.f;

    static vec3_t transform(const mat3_t& m, const vec3_t& v) {
        vec3_t r;
        r.x = m.v00 * v.x + m.v01 * v.y + m.v02 * v.z;
        r.y = m.v10 * v.x + m.v11 * v.y + m.v12 * v.z;
        r.z = m.v20 * v.x + m.v21 * v.y + m.v22 * v.z;
        return r;
    }
  };

static DVHSTX display;
static PicoGraphics_PenDVHSTX_RGB565 graphics(FRAME_WIDTH, FRAME_HEIGHT, display);

// Should be odd
#define NUM_POINTS_PER_AXIS 7
#define AXIS_EXTENT (NUM_POINTS_PER_AXIS >> 1)
#define NUM_POINTS (NUM_POINTS_PER_AXIS*NUM_POINTS_PER_AXIS*NUM_POINTS_PER_AXIS)
struct ColourPoint
{
    float z;
    Point p;
    RGB565 c;
};
ColourPoint pts[NUM_POINTS];
std::array<uint16_t, NUM_POINTS> pts_order;

void clear_screen() {
    graphics.set_pen(graphics.create_pen(0, 0, 0));
    graphics.clear();
}

void init_pts() {
    for (int r = 0, i = 0; r < NUM_POINTS_PER_AXIS; ++r) {
      for (int g = 0; g < NUM_POINTS_PER_AXIS; ++g) {
        for (int b = 0; b < NUM_POINTS_PER_AXIS; ++b, ++i) {
            pts[i].c = graphics.create_pen(r * (255/(NUM_POINTS_PER_AXIS-1)), g * (255/(NUM_POINTS_PER_AXIS-1)), b * (255/(NUM_POINTS_PER_AXIS-1)));
            pts_order[i] = i;
        }
      }
    }
}

void compute_pts(float t) {
    mat3_t r = mat3_t::rotationxy(t);
    r *= mat3_t::rotationxz(t*0.47f);
    r *= mat3_t::rotationyz(t*0.253f);
    for (int x = -AXIS_EXTENT, i = 0; x <= AXIS_EXTENT; ++x) {
      for (int y = -AXIS_EXTENT; y <= AXIS_EXTENT; ++y) {
        for (int z = -AXIS_EXTENT; z <= AXIS_EXTENT; ++z, ++i) {
            constexpr float m = 28.f;
            vec3_t v{ m*x, m*y, m*z };
            v = vec3_t::transform(r, v);
            pts[i].p.x = (int)(v.x + CENTRE_X);
            pts[i].p.y = (int)(v.y + CENTRE_Y);
            pts[i].z = v.z;
        }
      }
    }

    std::sort(pts_order.begin(), pts_order.end(), [](const uint16_t& a, const uint16_t& b) {
        return pts[a].z < pts[b].z;
    });
}

void display_pts() {
    for (int i = 0; i < NUM_POINTS; ++i) {
        const ColourPoint& pt = pts[pts_order[i]];
        //graphics.set_pen((pt.c >> 1) & 0x3DEF);
        //uint16_t c2 = (pt.c >> 1) & 0x39CF;
        //c2 *= 0x421 * 3;
        //c2 &= 0x7BDF;

        graphics.set_pen(pt.c);
        if (pt.z < 0) {
            graphics.circle(pt.p, 4);
            graphics.set_pen((pt.c >> 1) & 0x7BEF);
            graphics.circle(pt.p, 2);
        }
        else {
            graphics.circle(pt.p, 5);
            graphics.set_pen((pt.c >> 1) & 0x7BEF);
            graphics.circle(pt.p, 3);
        }
    }
}

int main()
{
    stdio_init_all();

    printf("Main\n");

    bool rv = display.init(FRAME_WIDTH, FRAME_HEIGHT);

    printf("Init complete: %s", rv ? "True" : "False");

    init_pts();

    clear_screen();
    display.flip_now();

    absolute_time_t start_time = get_absolute_time();
    absolute_time_t frame_start_time = start_time;

    while(true) {
        float t = absolute_time_diff_us(start_time, get_absolute_time()) * 0.0000005f;
        compute_pts(t);
        display.wait_for_flip();
        clear_screen();

        display_pts();

        char buffer[80];
        sprintf(buffer, "%.1fFPS", 1000000.f/absolute_time_diff_us(frame_start_time, get_absolute_time()));
        frame_start_time = get_absolute_time();
        graphics.set_pen(0xFFFF);
        graphics.text(buffer, {0,0}, FRAME_WIDTH, 1.0f);
        printf("%s\n", buffer);

        display.flip_async();
    }

    return 0;
}
