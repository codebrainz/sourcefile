#include <stdlib.h>
#include <stdio.h>
#include <sourcefile.h>

#include <gtk/gtk.h>


static GtkWidget *window = NULL;
static GtkWidget *scrollwin = NULL;
static GtkWidget *textview = NULL;


static GtkWidget *create_main_window (void)
{
  PangoFontDescription *pfd;

  if (window)
    return window;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Source File Test");
  gtk_window_set_default_size (GTK_WINDOW (window), 640, 480);
  g_signal_connect (window, "destroy", gtk_main_quit, NULL);

  scrollwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  textview = gtk_text_view_new ();
  pfd = pango_font_description_from_string ("Monospace 9");
  gtk_widget_modify_font (textview, pfd);
  pango_font_description_free (pfd);

  gtk_container_add (GTK_CONTAINER (scrollwin), textview);
  gtk_container_add (GTK_CONTAINER (window), scrollwin);

  gtk_widget_show_all (window);

  return window;
}


static void set_textview_text (SourceFile *file, const SourceFileBuffer *buf)
{
  GtkTextBuffer *buffer;

  if (!buf->data || !buf->length)
    return;

  buf = source_file_get_buffer (file);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
  gtk_text_buffer_set_text (buffer,
                            buf->data,
                            buf->length);
}


int main (int argc, char *argv[])
{
  SourceFile *file;
  const SourceFileBuffer *buffer;

  if (argc < 2)
    {
      fprintf (stderr, "not enough arguments\n");
      exit (EXIT_FAILURE);
    }

  gtk_init (&argc, &argv);

  create_main_window ();

  file = source_file_new (argv[1], NULL, NULL);
  buffer = source_file_get_buffer (file);

  printf ("Filename: %s\n"
          "Extension: %s\n"
          "Encoding: %s\n"
          "Content-Length: %ld\n"
          "Conent-Type: %s\n",
          source_file_get_filename (file),
          source_file_get_extension (file),
          source_file_get_charset (file),
          buffer->length,
          source_file_get_mime_type (file));

  set_textview_text (file, buffer);

  gtk_main ();

  g_object_unref (file);

  return 0;
}
