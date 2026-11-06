#ifndef CAMERA_H
#include "cairo.h"

typedef struct{
    double x;
    double y;
    double zoom;
}Camera;

void camera_init(Camera *c);
void camera_screen_to_coordinates(Camera *c, double sx, double sy, double *mx, double *my);
void camera_pan(Camera *c, double dx, double dy);
void camera_zoom_at(Camera *c, double sx, double sy, double factor);

#endif