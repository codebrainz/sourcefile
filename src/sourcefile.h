#ifndef __SOURCEFILE_H__
#define __SOURCEFILE_H__

#include <glib-object.h>

G_BEGIN_DECLS


#define SOURCE_FILE_FALLBACK_ENCODING     "ISO-8859-1"
#define SOURCE_FILE_CHARSET_PATTERN       \
  "(coding|charset|codepage)\\s*[:=]\\s*[\"']?\\s*([^'\"\\s]+)\\s*[\"']?"
#define SOURCE_FILE_CHARSET_PATTERN_MATCH 2


#define SOURCE_TYPE_FILE            (source_file_get_type ())
#define SOURCE_FILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SOURCE_TYPE_FILE, SourceFile))
#define SOURCE_FILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SOURCE_TYPE_FILE, SourceFileClass))
#define SOURCE_IS_FILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SOURCE_TYPE_FILE))
#define SOURCE_IS_FILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SOURCE_TYPE_FILE))
#define SOURCE_FILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SOURCE_TYPE_FILE, SourceFileClass))


typedef struct _SourceFile        SourceFile;
typedef struct _SourceFileClass   SourceFileClass;
typedef struct _SourceFilePrivate SourceFilePrivate;
typedef struct _SourceFileBuffer  SourceFileBuffer;


#if 0
typedef enum
{
  SOURCE_FILE_LINE_ENDING_AUTO,
  SOURCE_FILE_LINE_ENDING_CR,
  SOURCE_FILE_LINE_ENDING_LF,
  SOURCE_FILE_LINE_ENDING_CRLF
} SourceFileLineEnding;
#endif


struct _SourceFileBuffer
{
  gchar *data;
  gsize  length;
};


struct _SourceFile
{
  GObject             parent;
  SourceFilePrivate  *priv;
};


struct _SourceFileClass
{
  GObjectClass parent_class;
};


GType        source_file_get_type         (void);
SourceFile  *source_file_new              (const gchar  *filename,
                                           const gchar *charset,
                                           const gchar *mime_type);

const gchar *source_file_get_charset      (SourceFile   *file);
void         source_file_set_charset      (SourceFile   *file,
                                           const gchar  *charset);

const gchar *source_file_get_mime_type    (SourceFile   *file);
void         source_file_set_mime_type    (SourceFile   *file,
                                           const gchar  *mime_type);

const gchar *source_file_get_filename     (SourceFile   *file);
void         source_file_set_filename     (SourceFile   *file,
                                           const gchar  *filename);
const gchar *source_file_get_extension    (SourceFile   *file);

gboolean     source_file_save             (SourceFile   *file,
                                           const gchar  *filename);

gboolean     source_file_open             (SourceFile  *file,
                                           const gchar *filename,
                                           const gchar *charset,
                                           const gchar *mime_type);

gboolean     source_file_reload           (SourceFile   *file);

const SourceFileBuffer
            *source_file_get_buffer       (SourceFile   *file);

gboolean     source_file_get_contents     (SourceFile   *file,
                                           gchar       **contents,
                                           gsize        *length);
gboolean     source_file_set_contents     (SourceFile   *file,
                                           const gchar  *contents,
                                           gsize         length);


G_END_DECLS


#endif /* __SOURCEFILE_H__ */
