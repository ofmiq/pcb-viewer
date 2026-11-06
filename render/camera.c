#include "camera.h"

void camera_init(Camera *c){
    c->x = 0;
    c->y = 0;
    c->zoom = 1;
}

void camera_screen_to_coordinates(Camera *c, double sx, double sy, double *mx, double *my){
    *mx = c->x + sx/c->zoom;
    *my = c->y + sy/c->zoom;
}