#ifndef __SOURCECHARSETS_H__
#define __SOURCECHARSETS_H__

typedef struct _SourceFileCharset SourceFileCharset;

gboolean                 source_file_charset_exists (const gchar *charset_name);
const SourceFileCharset *source_file_lookup_charset (const gchar *charset_name);

#endif /* __SOURCECHARSETS_H__ */
