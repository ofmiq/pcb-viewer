#include "camera.h"

#define MIN_ZOOM 0.05
#define MAX_ZOOM 32.0

void camera_init(Camera *c) {
    c->x = 0;
    c->y = 0;
    c->zoom = 1;
}

void camera_screen_to_coordinates(const Camera* c, double sx, double sy, double* mx, double* my){
    *mx = c->x + sx / c->zoom;
    *my = c->y + sy / c->zoom;
}

void camera_pan(Camera* c, double dx, double dy){
    c->x -= dx / c->zoom;
    c->y -= dy /c->zoom;
}

void camera_zoom_at(Camera* c, double sx, double sy, double factor){
    double mx, my;
    camera_screen_to_coordinates(c, sx, sy, &mx, &my);

    double new_zoom = c->zoom * factor;
    if(new_zoom < MIN_ZOOM) new_zoom = MIN_ZOOM;
    if(new_zoom > MAX_ZOOM) new_zoom = MAX_ZOOM;
    c->zoom = new_zoom;

    c->x = mx - sx / new_zoom;
    c->y = my - sy / new_zoom;
}   