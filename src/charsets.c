#include <glib.h>
#include "charsets.h"


static SourceFileCharset **charset_table = NULL;
static guint charset_table_length = 0;


#if 0
static void
source_file_deinit_charset_table (void)
{
  guint i;

  for (i = 0; i < charset_table_length; i++)
    {
      g_free (charset_table[i]->name);
      g_free (charset_table[i]->mime_name);
      g_strfreev (charset_table[i]->aliases);
      g_free (charset_table[i]);
    }

  g_free (charset_table);
}
#endif


static void
source_file_init_charset_table (const gchar *filename)
{
  GKeyFile *kf;
  GError   *error;
  gsize     i, num_charsets;
  gchar   **groups;

  if (charset_table)
    return;

  kf = g_key_file_new ();

  error = NULL;
  if (!g_key_file_load_from_file (kf, filename, G_KEY_FILE_NONE, &error))
    {
      g_warning ("Failed to load character set data from file '%s': %s",
                  filename,
                  error->message);
      g_error_free (error);
      g_key_file_free (kf);
      return;
    }

  groups = g_key_file_get_groups (kf, &num_charsets);
  charset_table = g_new0 (SourceFileCharset*, num_charsets);
  charset_table_length = (guint) num_charsets;
  for (i = 0; i < num_charsets; i++)
    {
      charset_table[i] = g_new0 (SourceFileCharset, 1);
      charset_table[i]->name = g_strdup (groups[i]);
      error = NULL;
      charset_table[i]->mib_enum = g_key_file_get_integer (kf, groups[i], "mib_enum", &error);
      if (error)
        {
          charset_table[i]->mib_enum = -1;
          g_error_free (error);
        }
      charset_table[i]->mime_name = g_key_file_get_string (kf, groups[i], "mime_name", NULL);
      charset_table[i]->aliases =
        g_key_file_get_string_list (kf,
                                    groups[i],
                                    "aliases",
                                    &(charset_table[i]->n_aliases),
                                    NULL);

    }
  g_strfreev (groups);
  g_key_file_free (kf);
}


gboolean
source_file_charset_equals (const SourceFileCharset *charset, const gchar *charset_name)
{
  if (g_strcasecmp (charset->name, charset_name) == 0)
    return TRUE;
  else if (g_strcasecmp (charset->mime_name, charset_name) == 0)
    return TRUE;
  else
    {
      gsize i;
      for (i = 0; i < charset->n_aliases; i++)
        {
          if (g_strcasecmp (charset->aliases[i], charset_name) == 0)
            return TRUE;
        }
    }
  return FALSE;
}


gboolean
source_file_charset_exists (const gchar *charset_name)
{
  return source_file_lookup_charset (charset_name) != NULL;
}


const SourceFileCharset *
source_file_lookup_charset (const gchar *charset_name)
{
  guint i;

  source_file_init_charset_table (SOURCE_FILE_CHARSET_CONF);

  for (i = 0; i < charset_table_length; i++)
    {
      if (source_file_charset_equals (charset_table[i], charset_name))
        return (const SourceFileCharset *) charset_table[i];
    }

  return NULL;
}


gchar *
source_file_normalize_charset_name (const gchar *charset_name)
{
  const SourceFileCharset *charset;

  if (!charset_name)
    return NULL;

  charset = source_file_lookup_charset (charset_name);

  if (!charset)
    return NULL;

  return g_strdup (charset->name);
}






