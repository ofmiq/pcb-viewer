#include "cairo.h"

void draw_rectangles(cairo_t* cr){

    cairo_set_source_rgb(cr, 1, 0, 0);
    cairo_rectangle(cr, 0, 0, 60, 60);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 1, 0);
    cairo_rectangle(cr, 60, 0, 60, 60);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 0, 1);
    cairo_rectangle(cr, 0, 60, 60, 60);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_rectangle(cr, 60, 60, 60, 60);
    cairo_fill(cr);
}

void draw_lines(cairo_t* cr){
    cairo_set_source_rgb(cr, 1, 1, 1);

    cairo_move_to(cr, 0, 0);
    cairo_line_to(cr, 120,120);
    
    cairo_move_to(cr, 60, 0);
    cairo_line_to(cr, 120, 60);

    cairo_move_to(cr, 0, 60);
    cairo_line_to(cr, 60, 120);

    cairo_stroke(cr);
}

int main(){

    double zoom = 1.5;
    double cam_x = 40;
    double cam_y = 60;

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 200, 200); // create surface where we draw
    cairo_t* cr = cairo_create(surface); // create pen for drawing

    cairo_set_source_rgb(cr, 0, 0, 0); //set color of pen (pen, red, green, blue) rgb seting from 0.0 to 1.0
    cairo_rectangle(cr, 0, 0, 200, 200); //creating rectangle (pen, x, y, width, height)
    cairo_fill(cr);

    cairo_translate(cr, -cam_x, -cam_y);
    cairo_scale(cr, zoom , zoom);

    draw_rectangles(cr);
    draw_lines(cr);

    cairo_surface_write_to_png(surface, "picture.png"); // save picture to file
    cairo_destroy(cr); //deleting pointer to pen
    cairo_surface_destroy(surface); //deleting pointer to surface
    return 0;
}