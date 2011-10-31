#!/usr/bin/env python

"""
Dirty hack to parse IANA charset names file:
http://www.iana.org/assignments/character-sets
"""

CHARSET_FILE_PATH = "data/iana-charsets.txt"


import os, re, sys
import datetime

fobj = open (CHARSET_FILE_PATH, "r")

charsets = []
d = {}

def parse_name (line):
  line = line.strip ()
  is_mime_match = "(preferred MIME name)" in line
  m = re.match (r'^Name:\s+(.*?)\s*(\(.*?\))?\s*(\[.*?\])?\s*$', line)
  if m:
    return (m.group (1), is_mime_match)

def parse_alias (line):
  line = line.strip ()
  is_mime_match = "(preferred MIME name)" in line
  m = re.match (r'^Alias:\s*([^(]+)', line)
  if m:
    return (m.group (1), is_mime_match)


name_cnt = 0
max_aliases = 0
max_charset_len = 0

for line in fobj:
  if line.startswith ("Name:"):
    name_cnt += 1
    d = {}
    name, is_mime_name = parse_name (line)
    d["name"] = name
    if is_mime_name:
      d["mime_name"] = name
    charsets.append (d)
    if len (name) > max_charset_len:
      max_charset_len = len (name)
  if line.startswith ("MIBenum:"):
    d["mib_enum"] = int (line.split (':')[1].strip ())
  if line.startswith ("Alias:"):
    alias, is_mime_name = parse_alias (line)
    if alias and alias != "None":
      if "aliases" in d:
        d["aliases"].append (alias)
        if len (d["aliases"]) > max_aliases:
          max_aliases = len (d["aliases"])
      else:
        d["aliases"] = [alias]
      if is_mime_name:
        d["mime_name"] = alias


count = -1

time_stamp = datetime.datetime.now().strftime ("%d-%m-%Y %H:%M:%S")

print '#'
print '# Automatically generated from the IANA charset assignments:'
print '#   http://www.iana.org/assignments/character-sets'
print '#'
print '# Statistics:'
print '#   Number of charsets: %d' % name_cnt
print '#   Most aliases for a charset: %d' % max_aliases
print '#   Longest charset name: %d' % max_charset_len
print '#   Date generated: %s' % time_stamp
print '#'

for charset in charsets:
  
  count += 1
  
  if "aliases" in charset and charset["aliases"] and charset["aliases"][0]:
    charset["aliases"] = [ a.strip () for a in charset["aliases"] ]
    aliases_text = ';'.join (charset["aliases"])
  else:
    aliases_text = ''
  
  if "mib_enum" in charset:
    mib_ = charset["mib_enum"]
  else:
    mib_ = -1
  
  if "mime_name" in charset and charset["mime_name"]:
    mime_name = charset["mime_name"].strip ()
  else:
    mime_name = ''
  
  print ''
  print '[%s]' % charset["name"].strip ()
  print 'mib_enum=%d' % mib_
  print 'mime_name=%s' % mime_name
  print 'aliases=%s' % aliases_text
  
  #print ''
  #print '  /* %d: %s */' % (count, charset["name"])
  
  #if "mib_enum" in charset:
    #print '  charset_table[%d].mib_enum = %d;' % (count, charset["mib_enum"])
  #else:
    #print '  charset_table[%d].mib_enum = -1;' % count
  
  #print '  charset_table[%d].name = "%s";' % (count, charset["name"])
  
  #if "mime_name" in charset and charset["mime_name"]:
    #print '  charset_table[%d].mime_name = "%s";' % (count, charset["mime_name"])
  #else:
    #print '  charset_table[%d].mime_name = NULL;' % count
  
  #if "aliases" in charset and charset["aliases"] and len(charset["aliases"]) > 0 and charset["aliases"][0]:
    #i = 0
    #for al in charset["aliases"]:
      #print '  charset_table[%d].aliases[%d] = "%s";' % (count, i, al.strip ())
      #i += 1
    #print '  charset_table[%d].aliases[%d] = NULL;' % (count, i)
  #else:
    #print '  charset_table[%d].aliases[%d] = NULL;' % (count, 0)

































