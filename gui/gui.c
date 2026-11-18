#include "gui.h"

static GtkWidget *window = NULL;
static cairo_surface_t *bg_surface = NULL;

static double zoom = 1.0;
static double pan_x = 0.0;
static double pan_y = 0.0;
static double cur_mouse_x = 0.0;
static double cur_mouse_y = 0.0;
static double pan_start_x = 0.0;
static double pan_start_y = 0.0;
static gboolean is_centered = FALSE;

static void clamp_pan (int width, int height) {
  if (!bg_surface) return;

  double img_w = cairo_image_surface_get_width (bg_surface) * zoom;
  double img_h = cairo_image_surface_get_height (bg_surface) * zoom;

  if (img_w <= width) {
    pan_x = (width - img_w) / 2.0;
  } else {
    if (pan_x > 0.0) pan_x = 0.0;
    if (pan_x < width - img_w) pan_x = width - img_w;
  }

  if (img_h <= height) {
    pan_y = (height - img_h) / 2.0;
  } else {
    if (pan_y > 0.0) pan_y = 0.0;
    if (pan_y < height - img_h) pan_y = height - img_h;
  }
}

static void on_draw (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data) {
  if (!is_centered && bg_surface) {
    double fit_x = (double)width / cairo_image_surface_get_width (bg_surface);
    double fit_y = (double)height / cairo_image_surface_get_height (bg_surface);
    zoom = (fit_x < fit_y) ? fit_x : fit_y;
    is_centered = TRUE;
  }

  clamp_pan (width, height);

  cairo_save (cr);
  cairo_translate (cr, pan_x, pan_y);
  cairo_scale (cr, zoom, zoom);

  if (bg_surface) {
    cairo_set_source_surface (cr, bg_surface, 0, 0);
    cairo_paint (cr);
  }

  cairo_restore (cr);
}

static void on_motion (GtkEventControllerMotion *ctrl, double x, double y, gpointer user_data) {
  cur_mouse_x = x;
  cur_mouse_y = y;
}

static gboolean on_scroll (GtkEventControllerScroll *ctrl, double dx, double dy, gpointer user_data) {
  if (!bg_surface) return TRUE;

  GtkWidget *widget = GTK_WIDGET (user_data);
  int w = gtk_widget_get_width (widget);
  int h = gtk_widget_get_height (widget);

  double fit_x = (double)w / cairo_image_surface_get_width (bg_surface);
  double fit_y = (double)h / cairo_image_surface_get_height (bg_surface);
  double min_zoom = (fit_x < fit_y) ? fit_x : fit_y;

  double old_zoom = zoom;
  if (dy < 0) zoom *= 1.15;
  else if (dy > 0) zoom /= 1.15;

  if (zoom < min_zoom) zoom = min_zoom;
  if (zoom > 15.0) zoom = 15.0;

  pan_x = cur_mouse_x - (cur_mouse_x - pan_x) * (zoom / old_zoom);
  pan_y = cur_mouse_y - (cur_mouse_y - pan_y) * (zoom / old_zoom);
  clamp_pan (w, h);

  gtk_widget_queue_draw (widget);
  return TRUE;
}

static void on_pan_begin (GtkGestureDrag *gesture, double x, double y, gpointer user_data) {
  pan_start_x = pan_x;
  pan_start_y = pan_y;
}

static void on_pan_update (GtkGestureDrag *gesture, double offset_x, double offset_y, gpointer user_data) {
  GtkWidget *widget = GTK_WIDGET (user_data);
  pan_x = pan_start_x + offset_x;
  pan_y = pan_start_y + offset_y;
  clamp_pan (gtk_widget_get_width (widget), gtk_widget_get_height (widget));
  gtk_widget_queue_draw (widget);
}

static void on_window_destroy (GtkWidget *widget, gpointer user_data) {
  g_clear_pointer (&bg_surface, cairo_surface_destroy);
  is_centered = FALSE;
}

GtkWidget *do_image (GtkApplication* app) {
  GtkWidget *drawing_area;

  if (!window) {
    window = gtk_application_window_new (app);

    gtk_window_set_title (GTK_WINDOW (window), "PCB Viewer");
    gtk_window_set_default_size (GTK_WINDOW (window), 1000, 700);
    g_object_add_weak_pointer (G_OBJECT (window), (gpointer*)&window);
    g_signal_connect (window, "destroy", G_CALLBACK (on_window_destroy), NULL);

    const char *path = g_file_test ("gui/resources/standart.png", G_FILE_TEST_EXISTS)
                       ? "gui/resources/standart.png"
                       : "resources/standart.png";
    cairo_surface_t *raw_bg = cairo_image_surface_create_from_png (path);

    int orig_w = 600;
    int orig_h = 400;
    if (cairo_surface_status (raw_bg) == CAIRO_STATUS_SUCCESS) {
      orig_w = cairo_image_surface_get_width (raw_bg);
      orig_h = cairo_image_surface_get_height (raw_bg);
    } else {
      g_printerr ("Failed to load image (%s): %s\n", path,
                  cairo_status_to_string (cairo_surface_status (raw_bg)));
      g_clear_pointer (&raw_bg, cairo_surface_destroy);
    }

    const double max_w = 900.0;
    const double max_h = 650.0;
    double scale = 1.0;
    if (orig_w > max_w || orig_h > max_h) {
      double scale_x = max_w / orig_w;
      double scale_y = max_h / orig_h;
      scale = (scale_x < scale_y) ? scale_x : scale_y;
    }

    int disp_w = (int)(orig_w * scale);
    int disp_h = (int)(orig_h * scale);

    if (raw_bg) {
      bg_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, disp_w, disp_h);
      cairo_t *scale_cr = cairo_create (bg_surface);
      cairo_scale (scale_cr, scale, scale);
      cairo_set_source_surface (scale_cr, raw_bg, 0, 0);
      cairo_paint (scale_cr);
      cairo_destroy (scale_cr);
      cairo_surface_destroy (raw_bg);
    }

    drawing_area = gtk_drawing_area_new ();
    gtk_widget_set_hexpand (drawing_area, TRUE);
    gtk_widget_set_vexpand (drawing_area, TRUE);
    gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (drawing_area), on_draw, NULL, NULL);
    gtk_window_set_child (GTK_WINDOW (window), drawing_area);

    GtkEventController *motion = gtk_event_controller_motion_new ();
    g_signal_connect (motion, "motion", G_CALLBACK (on_motion), NULL);
    gtk_widget_add_controller (drawing_area, motion);

    GtkEventController *scroll = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
    g_signal_connect (scroll, "scroll", G_CALLBACK (on_scroll), drawing_area);
    gtk_widget_add_controller (drawing_area, scroll);

    GtkGesture *pan_drag = gtk_gesture_drag_new ();
    gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (pan_drag), GDK_BUTTON_PRIMARY);
    g_signal_connect (pan_drag, "drag-begin", G_CALLBACK (on_pan_begin), drawing_area);
    g_signal_connect (pan_drag, "drag-update", G_CALLBACK (on_pan_update), drawing_area);
    gtk_widget_add_controller (drawing_area, GTK_EVENT_CONTROLLER (pan_drag));

    if (!gtk_widget_get_visible (window)) {
      gtk_widget_set_visible (window, TRUE);
    }
  }
  else {
    gtk_window_destroy (GTK_WINDOW (window));
  }

  return window;
}

int show_image (int argc, char **argv) {
  GtkApplication *app;
  int status;

  app = gtk_application_new ("viewer.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (do_image), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}