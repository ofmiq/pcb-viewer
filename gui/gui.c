#include "gui.h"

static GtkWidget *window = NULL;

GtkWidget *do_image (GtkWidget *do_widget)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *picture;

  if (!window) {
      window = gtk_window_new ();
      gtk_window_set_display (GTK_WINDOW (window),
                              gtk_widget_get_display (do_widget));
      gtk_window_set_title (GTK_WINDOW (window), "Images");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer*)&window);

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
      gtk_widget_set_margin_start (vbox, 16);
      gtk_widget_set_margin_end (vbox, 16);
      gtk_widget_set_margin_top (vbox, 16);
      gtk_widget_set_margin_bottom (vbox, 16);
      gtk_window_set_child (GTK_WINDOW (window), vbox);

    
      frame = gtk_frame_new (NULL);
      gtk_widget_set_halign (frame, GTK_ALIGN_CENTER);
      gtk_widget_set_valign (frame, GTK_ALIGN_CENTER);
      gtk_box_append (GTK_BOX (vbox), frame);

      picture = gtk_picture_new_for_filename("pcb-viewer/resources/standart.png");
      gtk_picture_set_content_fit (GTK_PICTURE (picture), GTK_CONTENT_FIT_CONTAIN);
      gtk_frame_set_child (GTK_FRAME (frame), picture);

      if (!gtk_widget_get_visible (window)) {
        gtk_widget_set_visible (window, TRUE);
      }
      else {
        gtk_window_destroy (GTK_WINDOW (window));
      }
  }

  return window;
}

int show_image(int argc, char *argv[]) {
  GtkApplication *app;
  int status;

  app = gtk_application_new("viewer.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK(do_image), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  
  return status;
}
