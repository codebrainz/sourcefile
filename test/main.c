#include <stdlib.h>
#include <stdio.h>
#include <sourcefile.h>


int main (int argc, char *argv[])
{
  SourceFile *file;
  const SourceFileBuffer *buffer;

  if (argc < 2)
    {
      fprintf (stderr, "not enough arguments\n");
      exit (EXIT_FAILURE);
    }

  g_type_init ();

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

  g_object_unref (file);

  return 0;
}
