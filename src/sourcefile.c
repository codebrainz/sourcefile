#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include "sourcefile.h"


#ifdef HAVE_UCHARDET
#  include <uchardet.h>
#endif

#ifdef HAVE_MAGIC
#  include <magic.h>
#endif


#include "charsets.h"


struct _SourceFilePrivate
{
  gchar            *filename;
  gchar            *charset;
  gchar            *mime_type;
  SourceFileBuffer *buffer;
  gboolean          externally_modified;
  GRegex           *regex_charset;
  GFile            *file;
  GFileMonitor     *file_monitor;
  guint             file_handler_id;
};


enum
{
  SIGNAL_EXTERNALLY_MODIFIED,
  SIGNAL_LAST
};


static guint source_file_signals[SIGNAL_LAST] = { 0 };


static void     source_file_finalize             (GObject *object);
#ifndef HAVE_UCHARDET
static gchar   *source_file_scan_unicode_bom      (const gchar *buffer, gsize length);
#endif
static gchar   *source_file_guess_charset         (SourceFile *file, const gchar *buffer, gsize length);
static gchar   *source_file_guess_mime_type       (SourceFile *file, const gchar *buffer, gsize length);
#if 0
static SourceFileLineEnding
                source_file_guess_line_endings    (SourceFile *file);
#endif
static gboolean source_file_store_buffer          (SourceFile *file);
static gboolean source_file_load_buffer           (SourceFile *file);


G_DEFINE_TYPE(SourceFile, source_file, G_TYPE_OBJECT)


static void
source_file_class_init (SourceFileClass *klass)
{
  GObjectClass *g_object_class;

  g_object_class = G_OBJECT_CLASS(klass);
  g_object_class->finalize = source_file_finalize;
  g_type_class_add_private((gpointer)klass, sizeof(SourceFilePrivate));

  source_file_signals[SIGNAL_EXTERNALLY_MODIFIED] =
    g_signal_newv ("externally-modified",
                   G_TYPE_FROM_CLASS (g_object_class),
                   G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                   NULL,
                   NULL,
                   NULL,
                   g_cclosure_marshal_VOID__VOID,
                   G_TYPE_NONE,
                   0,
                   NULL);
}


static void
source_file_finalize (GObject *object)
{
  SourceFile *self;

  g_return_if_fail(object != NULL);
  g_return_if_fail(SOURCE_IS_FILE(object));

  self = SOURCE_FILE(object);

  /* cleanup resources */
  g_free (self->priv->charset);
  g_free (self->priv->mime_type);
  g_free (self->priv->filename);
  g_free (self->priv->buffer->data);
  g_free (self->priv->buffer);

  if (self->priv->regex_charset)
    g_regex_unref (self->priv->regex_charset);

  if (self->priv->file)
    g_object_unref (self->priv->file);

  G_OBJECT_CLASS(source_file_parent_class)->finalize(object);
}


static void source_file_init(SourceFile *self)
{
  GError *error;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            SOURCE_TYPE_FILE,
                                            SourceFilePrivate);

  /* set defaults */
  self->priv->charset         = NULL;
  self->priv->mime_type       = NULL;
  self->priv->filename        = NULL;
  self->priv->buffer          = g_new0 (SourceFileBuffer, 1);
  self->priv->buffer->data    = NULL;
  self->priv->buffer->length  = 0;
  self->priv->file            = NULL;
  self->priv->file_handler_id = 0;

  /* pre-compile regular expression(s) */
  error = NULL;
  self->priv->regex_charset = g_regex_new (SOURCE_FILE_CHARSET_PATTERN,
                                           G_REGEX_CASELESS,
                                           G_REGEX_MATCH_NEWLINE_ANY,
                                           &error);
  if (error)
    {
      g_warning ("Failed to compile regular expression pattern: %s", error->message);
      g_error_free (error);
      self->priv->regex_charset = NULL;
    }
}


#ifndef HAVE_UCHARDET
static gchar *
source_file_scan_unicode_bom (const gchar *buffer, gsize length)
{

  if (length >= 3)
    {
      if ((guchar)buffer[0] == 0xef &&
          (guchar)buffer[1] == 0xbb &&
          (guchar)buffer[2] == 0xbf)
        {
          return g_strdup ("UTF-8");
        }
    }

  if (length >= 4)
    {
      if ((guchar)buffer[0] == 0x00 &&
          (guchar)buffer[1] == 0x00 &&
				  (guchar)buffer[2] == 0xfe &&
          (guchar)buffer[3] == 0xff)
        {
          return g_strdup ("UTF-32BE");
        }

      if ((guchar)buffer[0] == 0xff &&
          (guchar)buffer[1] == 0xfe &&
          (guchar)buffer[2] == 0x00 &&
          (guchar)buffer[3] == 0x00)
        {
          return g_strdup ("UTF-32LE");
        }

      if ((buffer[0] == 0x2b &&
           buffer[1] == 0x2f &&
           buffer[2] == 0x76) &&
          (buffer[3] == 0x38 ||
           buffer[3] == 0x39 ||
           buffer[3] == 0x2b ||
           buffer[3] == 0x2f))
        {
          return g_strdup ("UTF-7");
        }
    }

	if (length >= 2)
    {
      if ((guchar)buffer[0] == 0xfe &&
          (guchar)buffer[1] == 0xff)
        {
          return g_strdup ("UTF-16BE");
        }

      if ((guchar)buffer[0] == 0xff &&
          (guchar)buffer[1] == 0xfe)
        {
          return g_strdup ("UTF-16LE");
        }
    }

	return NULL;
}
#endif


static gchar *
source_file_guess_charset (SourceFile *file, const gchar *buffer, gsize length)
{
  gchar      *charset = NULL;
  GMatchInfo *info;
  GError     *error;
  const SourceFileCharset *cs;

  g_return_val_if_fail (SOURCE_IS_FILE (file), NULL);
  g_return_val_if_fail (buffer, NULL);
  g_return_val_if_fail (length, NULL);

  /* TODO: could limit to the start/end of the file */
  info = NULL;
  error = NULL;
  if (g_regex_match_full (file->priv->regex_charset,
                          buffer, length,
                          0, 0, &info, &error))
    {
      charset = g_match_info_fetch (info, SOURCE_FILE_CHARSET_PATTERN_MATCH);
      if (!charset || strlen (charset) == 1)
        {
          g_free (charset);
          charset = NULL;
        }
    }

  g_match_info_free (info);

  if (error)
    {
      g_warning ("Error matching regular expression: %s", error->message);
      g_error_free (error);
    }

  /* since the regular expressions can match arbitrary strings, we
   * need to make sure it's a real charset name */
  if (charset)
    {
      if (!source_file_charset_exists (charset))
        {
          g_warning ("Detected charset name '%s' is not known so "
                     "is not being used", charset);
          g_free (charset);
          charset = NULL;
        }
    }

  if (!charset)
    {
#ifdef HAVE_UCHARDET
      uchardet_t   ud;
      const gchar *cs;
      ud = uchardet_new ();
      uchardet_handle_data (ud, buffer, length);
      uchardet_data_end (ud);
      cs = uchardet_get_charset (ud);
      if (cs && strlen (cs))
        charset = g_strdup (cs);
      uchardet_delete (ud);
#else
      charset = source_file_scan_unicode_bom (file->priv->buffer->data,
                                              file->priv->buffer->length);
#endif
    }

  if (!charset)
    {
      gchar *s;
      if (((s = getenv ("LC_ALL")) && *s) ||
          ((s = getenv ("LC_CTYPE")) && *s) ||
          ((s = getenv ("LANG")) && *s))
        {
          if (strstr (s, "UTF-8"))
            charset = g_strdup ("UTF-8");
        }
    }

  if (!charset)
    charset = g_strdup (SOURCE_FILE_FALLBACK_CHARSET);

  /* normalize the name */
  cs = source_file_lookup_charset (charset);
  if (cs)
    {
      g_free (charset);
      charset = g_strdup (cs->name);
    }

  return charset;
}


static gchar *
source_file_guess_mime_type (SourceFile *file, const gchar *buffer, gsize length)
{
  gchar *mime_type = NULL;

#ifdef HAVE_MAGIC
  const gchar *mbuf;
  magic_t      cookie;

  cookie = magic_open (MAGIC_MIME_TYPE);
  mbuf = magic_buffer (cookie, buffer, length);
  if (mbuf)
    mime_type = g_strdup (mbuf);

  magic_close (cookie);
#endif

  if (!mime_type)
    {
      gchar    *content_type;
      gboolean  result_uncertain;

      content_type = g_content_type_guess (file->priv->filename,
                                           (const guchar *) buffer,
                                           length,
                                           &result_uncertain);

      if (!result_uncertain)
        mime_type = g_content_type_get_mime_type (content_type);

      g_free (content_type);
    }

  return mime_type;
}


#if 0
static SourceFileLineEnding
source_file_guess_line_endings (SourceFile *file)
{
#define LIMIT_LINES 50

  gsize i;
  gint  cr, lf, crlf, lines, max_mode;
  SourceFileLineEnding eol;

  cr = lf = crlf = lines = max_mode = 0;

  for (i = 0; i < file->priv->buffer->length; i++)
    {
      switch (file->priv->buffer->data[i])
        {
        case '\r':
          if (i < file->priv->buffer->length - 2)
            {
              if (file->priv->buffer->data[i + 1] == '\n')
                {
                  crlf++;
                  i++;
                }
              else
                cr++;
            }
          else
            cr++;
          lines++;
        case '\n':
          lf++;
          lines++;
        }
      if (lines >= LIMIT_LINES)
        break;
    }

  eol = SOURCE_FILE_LINE_ENDING_LF;
  max_mode = lf;

  if (cr > max_mode)
    {
      eol = SOURCE_FILE_LINE_ENDING_CR;
      max_mode = cr;
    }

  if (crlf > max_mode)
    eol = SOURCE_FILE_LINE_ENDING_CRLF;

  return eol;

#undef LIMIT_LINES
}
#endif


static gboolean
source_file_store_buffer (SourceFile *file)
{
  gsize   length;
  gchar  *buffer;
  GFile  *gfile;
  GError *error;

  g_return_val_if_fail (SOURCE_IS_FILE (file), FALSE);
  g_return_val_if_fail (file->priv->filename, FALSE);

  gfile = g_file_new_for_path (file->priv->filename);

  if (!gfile)
    return FALSE;

  error = NULL;
  buffer = g_convert_with_fallback (file->priv->buffer->data,
                                    file->priv->buffer->length,
                                    "UTF-8",
                                    file->priv->charset,
                                    NULL,
                                    NULL,
                                    &length,
                                    &error);

  if (!buffer)
    {
      g_warning ("Failed to convert buffer to '%s': %s", file->priv->charset, error->message);
      g_error_free (error);
      g_object_unref (gfile);
      return FALSE;
    }

  error = NULL;
  if (!g_file_replace_contents (gfile,
                                buffer, length,
                                NULL, FALSE,
                                G_FILE_CREATE_NONE,
                                NULL, NULL,
                                &error))
    {
      g_warning ("Failed to store file '%s': %s", file->priv->filename, error->message);
      g_error_free (error);
      g_free (buffer);
      g_object_unref (gfile);
      return FALSE;
    }

  g_object_unref (gfile);
  g_free (buffer);

  return TRUE;
}


static gboolean
source_file_load_buffer (SourceFile *file)
{
  gsize   length;
  gchar  *buffer;
  GFile  *gfile;
  GError *error;

  g_return_val_if_fail (SOURCE_IS_FILE (file), FALSE);
  g_return_val_if_fail (file->priv->filename, FALSE);
  g_return_val_if_fail (g_file_test (file->priv->filename, G_FILE_TEST_EXISTS), FALSE);

  g_free (file->priv->buffer->data);
  file->priv->buffer->data = NULL;
  file->priv->buffer->length = 0;

  gfile = g_file_new_for_path (file->priv->filename);
  if (!gfile)
    return FALSE;

  error = NULL;
  if (!g_file_load_contents (gfile, NULL, &buffer, &length, NULL, &error))
    {
      g_warning ("Failed to load file '%s': %s", file->priv->filename, error->message);
      g_error_free (error);
      g_object_unref (gfile);
      return FALSE;
    }

  g_object_unref (gfile);

  if (!file->priv->charset)
    {
      g_free (file->priv->charset);
      file->priv->charset = source_file_guess_charset (file, buffer, length);
    }

  if (!file->priv->mime_type)
    {
      g_free (file->priv->mime_type);
      file->priv->mime_type = source_file_guess_mime_type (file, buffer, length);
    }

  error = NULL;
  file->priv->buffer->data =
    g_convert_with_fallback (buffer,
                             length,
                             file->priv->charset,
                             "UTF-8",
                             NULL,
                             NULL,
                             &(file->priv->buffer->length),
                             &error);

  if (error)
    {
      g_warning ("Failed to convert buffer from '%s': %s", file->priv->charset, error->message);
      g_error_free (error);
      g_free (buffer);
      return FALSE;
    }

  g_free (buffer);

  return TRUE;
}


SourceFile *
source_file_new (const gchar *filename, const gchar *charset, const gchar *mime_type)
{
  SourceFile *file;

  file = SOURCE_FILE (g_object_new (SOURCE_TYPE_FILE, NULL));

  if (filename)
    source_file_set_filename (file, filename);

  if (charset)
    source_file_set_charset (file, charset);

  if (mime_type)
    source_file_set_mime_type (file, mime_type);

  if (filename)
    source_file_load_buffer (file);

  return file;
}


const SourceFileBuffer *
source_file_get_buffer (SourceFile *file)
{
  g_return_val_if_fail (SOURCE_IS_FILE (file), NULL);
  return (const SourceFileBuffer *) file->priv->buffer;
}


gboolean
source_file_get_contents (SourceFile *file, gchar **buffer, gsize *length)
{
  g_return_val_if_fail (SOURCE_IS_FILE (file), FALSE);
  g_return_val_if_fail (buffer, FALSE);

  if (buffer && file->priv->buffer->data)
    {
      *buffer = g_malloc0 (file->priv->buffer->length);
      memcpy (*buffer, file->priv->buffer->data, file->priv->buffer->length);
    }
  else if (buffer)
    *buffer = NULL;

  if (length)
    *length = file->priv->buffer->length;

  return TRUE;
}


gboolean
source_file_set_contents (SourceFile *file, const gchar *buffer, gsize length)
{
  g_return_val_if_fail (SOURCE_IS_FILE (file), FALSE);

  g_free (file->priv->buffer);
  file->priv->buffer->data = NULL;
  file->priv->buffer->length = 0;

  if (buffer)
    {
      file->priv->buffer->data = g_malloc0 (length);
      file->priv->buffer->length = length;
      memcpy (file->priv->buffer->data, buffer, length);
    }

  return TRUE;
}


gboolean
source_file_reload (SourceFile *file)
{
  return source_file_load_buffer (file);
}


gboolean
source_file_open (SourceFile  *file,
                  const gchar *filename,
                  const gchar *charset,
                  const gchar *mime_type)
{
  g_return_val_if_fail (SOURCE_IS_FILE (file), FALSE);
  g_return_val_if_fail (filename, FALSE);

  source_file_set_filename (file, filename);

  g_free (file->priv->charset);
  file->priv->charset = NULL;

  if (charset)
    file->priv->charset = g_strdup (charset);

  g_free (file->priv->mime_type);
  file->priv->mime_type = NULL;

  if (mime_type)
    file->priv->mime_type = g_strdup (mime_type);

  return source_file_load_buffer (file);
}


gboolean
source_file_save (SourceFile *file, const gchar *filename)
{
  g_return_val_if_fail (SOURCE_IS_FILE (file), FALSE);

  if (filename)
    source_file_set_filename (file, filename);

  return source_file_store_buffer (file);
}


const gchar *
source_file_get_charset (SourceFile *file)
{
  g_return_val_if_fail (SOURCE_IS_FILE (file), NULL);
  return (const gchar *) file->priv->charset;
}


void
source_file_set_charset (SourceFile *file, const gchar  *charset)
{
  g_return_if_fail (SOURCE_IS_FILE (file));
  g_return_if_fail (charset);

  g_free (file->priv->charset);
  file->priv->charset = source_file_normalize_charset_name (charset);

  if (!file->priv->charset)
    file->priv->charset = g_strdup (SOURCE_FILE_FALLBACK_CHARSET);
}


const gchar *
source_file_get_mime_type (SourceFile *file)
{
  g_return_val_if_fail (SOURCE_IS_FILE (file), NULL);
  return (const gchar *) file->priv->mime_type;
}


void
source_file_set_mime_type (SourceFile *file, const gchar  *mime_type)
{
  g_return_if_fail (SOURCE_IS_FILE (file));
  g_return_if_fail (mime_type);

  g_free (file->priv->mime_type);
  file->priv->mime_type = g_strdup (mime_type);
}


const gchar *
source_file_get_filename (SourceFile *file)
{
  g_return_val_if_fail (SOURCE_IS_FILE (file), NULL);
  return (const gchar *) file->priv->filename;
}


const gchar *
source_file_get_extension (SourceFile *file)
{
  GFile       *gfile;
  gchar       *base_name;
  const gchar *ext = NULL;

  g_return_val_if_fail (SOURCE_IS_FILE (file), NULL);

  if (!file->priv->filename)
    return NULL;

  gfile = g_file_new_for_path (file->priv->filename);
  base_name = g_file_get_basename (gfile);

  if (strchr (base_name, '.'))
    ext = (const gchar *) strrchr (file->priv->filename, '.');

  g_free (base_name);
  g_object_unref (gfile);

  return ext;
}


static void
on_file_monitor_changed (GFileMonitor      *monitor,
                         GFile             *gfile,
                         GFile             *other_file,
                         GFileMonitorEvent  event_type,
                         SourceFile        *file)
{
  g_return_if_fail (SOURCE_IS_FILE (file));

  switch (event_type)
    {
    case G_FILE_MONITOR_EVENT_CHANGED:
    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
    case G_FILE_MONITOR_EVENT_DELETED:
    case G_FILE_MONITOR_EVENT_MOVED:
      g_debug ("File '%s' was externally modified",
               source_file_get_filename (file));
      g_signal_emit_by_name (file, "externally-modified");
      break;
    /* skip others */
    default:
      break;
    }
}


void
source_file_set_filename (SourceFile *file, const gchar  *filename)
{
  g_return_if_fail (SOURCE_IS_FILE (file));
  g_return_if_fail (filename);

  if (g_strcmp0 (filename, file->priv->filename) == 0)
    return;

  g_free (file->priv->filename);
  file->priv->filename = g_strdup (filename);

  if (file->priv->file)
    g_object_unref (file->priv->file);

  if (file->priv->file_monitor)
    g_object_unref (file->priv->file_monitor);

  if (g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      file->priv->file = g_file_new_for_path (filename);

      if (G_IS_FILE (file->priv->file))
        {
          file->priv->file_monitor =
            g_file_monitor_file (file->priv->file,
                                 G_FILE_MONITOR_SEND_MOVED,
                                 NULL,
                                 NULL);


          if (G_IS_FILE_MONITOR (file->priv->file_monitor))
            {
              file->priv->file_handler_id =
                g_signal_connect (file->priv->file_monitor,
                                  "changed",
                                  G_CALLBACK (on_file_monitor_changed),
                                  file);
            }
        }
    }
}































