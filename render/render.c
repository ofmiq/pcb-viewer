#include <math.h>
#include <stdio.h>

#include "cairo.h"
#include "camera.h"

static int failures = 0;

static void check(const char* name, double got, double expected){
  int ok = fabs(got - expected) < 1e-9;
  if(!ok) failures++;
  printf("%s %s: got %f, expected %f\n", ok ? "OK " : "FAIL ", name, got, expected);
}

static void test_camera(void) {
  Camera c;
  double mx, my;

  c.x = 100; c.y = 50; c.zoom = 2;
  camera_screen_to_coordinates(&c, 10, 20, &mx, &my);
  check("screen_to_coords x", mx, 105);
  check("screen_to_coords y", my, 60);

  camera_init(&c);
  c.zoom = 0.5;
  camera_screen_to_coordinates(&c, 10, 10, &mx, &my);
  check("zoom 0.5 x", mx, 20);
  check("zoom 0.5 y", my, 20);

  c.x = 100; c.y = 50; c.zoom = 2;
  camera_pan(&c, 10, 0);
  check("pan x", c.x, 95);
  check("pan y", c.y, 50);
  camera_pan(&c, -10, 0);
  check("pan back x", c.x, 100);

  camera_init(&c);
  camera_zoom_at(&c, 100, 100, 2);
  check("zoom_at zoom", c.zoom, 2);
  check("zoom_at x", c.x, 50);
  check("zoom_at y", c.y, 50);

  double bx, by, ax, ay;
  c.x = 13; c.y = -7; c.zoom = 1.7;
  camera_screen_to_coordinates(&c, 250, 80, &bx, &by);
  camera_zoom_at(&c, 250, 80, 3.1);
  camera_screen_to_coordinates(&c, 250, 80, &ax, &ay);
  check("invariant x", ax, bx);
  check("invariant y", ay, by);

  camera_init(&c);
  camera_zoom_at(&c, 10, 10, 1000);
  check("max zoom", c.zoom, 32);
  camera_zoom_at(&c, 10, 10, 1e-6);
  check("min zoom", c.zoom, 0.05);
}

int main() {
  test_camera();

  if(failures == 0){
    printf("all test pased \n");
  }else{
    printf("%d test failed\n", failures);
  }
  return 0;
}
