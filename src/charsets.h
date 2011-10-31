#ifndef __SOURCECHARSETS_H__
#define __SOURCECHARSETS_H__

typedef struct _SourceFileCharset SourceFileCharset;

struct _SourceFileCharset
{
  gint    mib_enum;
  gchar  *name;
  gchar  *mime_name;
  gchar **aliases;
  gsize   n_aliases;
};

gboolean                 source_file_charset_exists (const gchar             *charset_name);
const SourceFileCharset *source_file_lookup_charset (const gchar             *charset_name);
gboolean                 source_file_charset_equals (const SourceFileCharset *charset,
                                                     const gchar             *charset_name);
gchar                   *source_file_normalize_charset_name (const gchar *charset_name);

#endif /* __SOURCECHARSETS_H__ */
