/* Needs:

   libunistring-dev
   cc wrap.c -lunistring -o a.out

*/

#include <iconv.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <strings.h>
#include <stdint.h>
#include <ctype.h>
#include <uniconv.h>
#include <unilbrk.h>

#define NFORMATS 28     /* Number of format_type enum values.  */

#define NSYNTAXCHECKS 4


enum
    {
     /* Set on the first byte of a format directive.  */
     FMTDIR_START  = 1 << 0,
     /* Set on the last byte of a format directive.  */
     FMTDIR_END    = 1 << 1,
     /* Set on the last byte of an invalid format directive, where a parse error
        was recognized.  */
     FMTDIR_ERROR  = 1 << 2
    };

/* Is current msgid a format string?  */
enum is_format
    {
     undecided,
     yes,
     no,
     yes_according_to_context,
     possible,
     impossible
    };

int
possible_format_p (enum is_format is_format)
{
    return is_format == possible
        || is_format == yes_according_to_context
        || is_format == yes;
}

int
possible_format_p (enum is_format);

enum
    {
     ATTR_ESCAPE_SEQUENCE          = 1 << 0,
     /* The following two are exclusive.  */
     ATTR_FORMAT_DIRECTIVE         = 1 << 1,
     ATTR_INVALID_FORMAT_DIRECTIVE = 1 << 2
    };


static const char ascii[] = "ASCII";

/* The canonicalized encoding name for ASCII.  */
const char *po_charset_ascii = ascii;

static const char utf8[] = "UTF-8";

/* The canonicalized encoding name for UTF-8.  */
const char *po_charset_utf8 = utf8;

int
c_strcasecmp (const char *s1, const char *s2);

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))

typedef struct s_buffer {
    size_t pos;
    size_t size;
    char *mem;
} buffer;

buffer *buffer_new(int len) {
    buffer *b = malloc(sizeof(buffer));
    b->mem = calloc(len, sizeof(*b->mem));
    b->pos = 0;
    b->size = len;
    return b;
}

void buffer_append(buffer *buffer, const char *str) {
    int length = strlen(str);

    while (length + buffer->pos >= buffer->size) {
        buffer->size *= 2;
        buffer->mem = realloc(buffer->mem, buffer->size);
    }
    memcpy(buffer->mem + buffer->pos, str, length);
    buffer->pos += length;
}

void buffer_append_char(buffer *buffer, char str) {
    int length = 1;

    while (length + buffer->pos >= buffer->size) {
        buffer->size *= 2;
        buffer->mem = realloc(buffer->mem, buffer->size);
    }
    buffer->mem[buffer->pos] = str;
    buffer->pos += length;
}

void buffer_del(buffer *buffer) {
    free(buffer->mem);
    free(buffer);
}

/* Test for a weird encoding, i.e. an encoding which has double-byte
   characters ending in 0x5C.  */
int po_is_charset_weird (const char *canon_charset)
{
    static const char *weird_charsets[] =
        {
         "BIG5",
         "BIG5-HKSCS",
         "GBK",
         "GB18030",
         "SHIFT_JIS",
         "JOHAB"
        };
    size_t i;

    for (i = 0; i < SIZEOF (weird_charsets); i++)
        if (strcmp (canon_charset, weird_charsets[i]) == 0)
            return 1;
    return 0;
}

/* Test for a weird CJK encoding, i.e. a weird encoding with CJK structure.
   An encoding has CJK structure if every valid character stream is composed
   of single bytes in the range 0x{00..7F} and of byte pairs in the range
   0x{80..FF}{30..FF}.  */
int po_is_charset_weird_cjk (const char *canon_charset)
{
    static const char *weird_cjk_charsets[] =
        {                     /* single bytes   double bytes       */
         "BIG5",             /* 0x{00..7F},    0x{A1..F9}{40..FE} */
         "BIG5-HKSCS",       /* 0x{00..7F},    0x{88..FE}{40..FE} */
         "GBK",              /* 0x{00..7F},    0x{81..FE}{40..FE} */
         "GB18030",          /* 0x{00..7F},    0x{81..FE}{30..FE} */
         "SHIFT_JIS",        /* 0x{00..7F},    0x{81..F9}{40..FC} */
         "JOHAB"             /* 0x{00..7F},    0x{84..F9}{31..FE} */
        };
    size_t i;

    for (i = 0; i < SIZEOF (weird_cjk_charsets); i++)
        if (strcmp (canon_charset, weird_cjk_charsets[i]) == 0)
            return 1;
    return 0;
}

const char *po_charset_canonicalize (const char *charset)
{
    /* The list of charsets supported by glibc's iconv() and by the portable
       iconv() across platforms.  Taken from intl/config.charset.  */
    static const char *standard_charsets[] =
        {
         "ASCII", "ANSI_X3.4-1968", "US-ASCII",        /* i = 0..2 */
         "ISO-8859-1", "ISO_8859-1",                 /* i = 3, 4 */
         "ISO-8859-2", "ISO_8859-2",
         "ISO-8859-3", "ISO_8859-3",
         "ISO-8859-4", "ISO_8859-4",
         "ISO-8859-5", "ISO_8859-5",
         "ISO-8859-6", "ISO_8859-6",
         "ISO-8859-7", "ISO_8859-7",
         "ISO-8859-8", "ISO_8859-8",
         "ISO-8859-9", "ISO_8859-9",
         "ISO-8859-13", "ISO_8859-13",
         "ISO-8859-14", "ISO_8859-14",
         "ISO-8859-15", "ISO_8859-15",               /* i = 25, 26 */
         "KOI8-R",
         "KOI8-U",
         "KOI8-T",
         "CP850",
         "CP866",
         "CP874",
         "CP932",
         "CP949",
         "CP950",
         "CP1250",
         "CP1251",
         "CP1252",
         "CP1253",
         "CP1254",
         "CP1255",
         "CP1256",
         "CP1257",
         "GB2312",
         "EUC-JP",
         "EUC-KR",
         "EUC-TW",
         "BIG5",
         "BIG5-HKSCS",
         "GBK",
         "GB18030",
         "SHIFT_JIS",
         "JOHAB",
         "TIS-620",
         "VISCII",
         "GEORGIAN-PS",
         "UTF-8"
        };
    size_t i;

    for (i = 0; i < SIZEOF (standard_charsets); i++)
        if (strcasecmp (charset, standard_charsets[i]) == 0)
            return standard_charsets[i < 3 ? 0 : i < 27 ? ((i - 3) & ~1) + 3 : i];
    return NULL;
}




int escape = 0;
int indent = 0;
int wrap_strings = 1;


char *
wrap (const char *line_prefix, int extra_indent,
      const char *name, const char *value,
      int do_wrap, size_t page_width,
      const char *charset)
{
    const char *canon_charset;
    char *fmtdir;
    const char *s;
    int first_line;
#if HAVE_ICONV
    const char *envval;
    iconv_t conv;
#endif
    int weird_cjk;
    buffer *out;
    char *output_string;

    out = buffer_new(1024);
    output_string = out->mem;

    canon_charset = po_charset_canonicalize (charset);

#if HAVE_ICONV
    /* The old Solaris/openwin msgfmt and GNU msgfmt <= 0.10.35 don't know
       about multibyte encodings, and require a spurious backslash after
       every multibyte character whose last byte is 0x5C.  Some programs,
       like vim, distribute PO files in this broken format.  It is important
       for such programs that GNU msgmerge continues to support this old
       PO file format when the Makefile requests it.  */
    envval = getenv ("OLD_PO_FILE_OUTPUT");
    if (envval != NULL && *envval != '\0')
        /* Write a PO file in old format, with extraneous backslashes.  */
        conv = (iconv_t)(-1);
    else
        if (canon_charset == NULL)
            /* Invalid PO file encoding.  */
            conv = (iconv_t)(-1);
        else
            /* Avoid glibc-2.1 bug with EUC-KR.  */
# if ((__GLIBC__ == 2 && __GLIBC_MINOR__ <= 1) && !defined __UCLIBC__)  \
    && !defined _LIBICONV_VERSION
            if (strcmp (canon_charset, "EUC-KR") == 0)
                conv = (iconv_t)(-1);
            else
# endif
                /* Avoid Solaris 2.9 bug with GB2312, EUC-TW, BIG5, BIG5-HKSCS, GBK,
                   GB18030.  */
# if defined __sun && !defined _LIBICONV_VERSION
                if (   strcmp (canon_charset, "GB2312") == 0
                       || strcmp (canon_charset, "EUC-TW") == 0
                       || strcmp (canon_charset, "BIG5") == 0
                       || strcmp (canon_charset, "BIG5-HKSCS") == 0
                       || strcmp (canon_charset, "GBK") == 0
                       || strcmp (canon_charset, "GB18030") == 0)
                    conv = (iconv_t)(-1);
                else
# endif
                    /* Use iconv() to parse multibyte characters.  */
                    conv = iconv_open ("UTF-8", canon_charset);

    if (conv != (iconv_t)(-1))
        weird_cjk = 0;
    else
#endif
        if (canon_charset == NULL)
            weird_cjk = 0;
        else
            weird_cjk = po_is_charset_weird_cjk (canon_charset);

    if (canon_charset == NULL)
        canon_charset = po_charset_ascii;

    /* Determine the extent of format string directives.  */
    fmtdir = NULL;
    /* Loop over the '\n' delimited portions of value.  */
    s = value;
    first_line = 1;
    do
        {
            /* The usual escapes, as defined by the ANSI C Standard.  */
#     define is_escape(c)                                               \
            ((c) == '\a' || (c) == '\b' || (c) == '\f' || (c) == '\n'   \
             || (c) == '\r' || (c) == '\t' || (c) == '\v')

            const char *es;
            const char *ep;
            size_t portion_len;
            char *portion;
            char *overrides;
            char *attributes;
            char *linebreaks;
            char *pp;
            char *op;
            char *ap;
            int startcol, startcol_after_break, width;
            size_t i;

            for (es = s; *es != '\0'; )
                if (*es++ == '\n')
                    break;

            /* Expand escape sequences in each portion.  */
            for (ep = s, portion_len = 0; ep < es; ep++)
                {
                    char c = *ep;
                    if (is_escape (c))
                        portion_len += 2;
                    else if (escape && !isprint ((unsigned char) c))
                        portion_len += 4;
                    else if (c == '\\' || c == '"')
                        portion_len += 2;
                    else
                        {
#if HAVE_ICONV
                            if (conv != (iconv_t)(-1))
                                {
                                    /* Skip over a complete multi-byte character.  Don't
                                       interpret the second byte of a multi-byte character as
                                       ASCII.  This is needed for the BIG5, BIG5-HKSCS, GBK,
                                       GB18030, SHIFT_JIS, JOHAB encodings.  */
                                    char scratchbuf[64];
                                    const char *inptr = ep;
                                    size_t insize;
                                    char *outptr = &scratchbuf[0];
                                    size_t outsize = sizeof (scratchbuf);
                                    size_t res;

                                    res = (size_t)(-1);
                                    for (insize = 1; inptr + insize <= es; insize++)
                                        {
                                            res = iconv (conv,
                                                         (ICONV_CONST char **) &inptr, &insize,
                                                         &outptr, &outsize);
                                            if (!(res == (size_t)(-1) && errno == EINVAL))
                                                break;
                                            /* We expect that no input bytes have been consumed
                                               so far.  */
                                            if (inptr != ep)
                                                abort ();
                                        }
                                    if (res == (size_t)(-1))
                                        {
                                            if (errno == EILSEQ)
                                                {
                                                    printf("invalid multibyte sequence\n");
                                                    continue;
                                                }
                                            else if (errno == EINVAL)
                                                {
                                                    /* This could happen if an incomplete
                                                       multibyte sequence at the end of input
                                                       bytes.  */
                                                    printf("incomplete multibyte sequence\n");
                                                    continue;
                                                }
                                            else
                                                abort ();
                                        }
                                    insize = inptr - ep;
                                    portion_len += insize;
                                    ep += insize - 1;
                                }
                            else
#endif
                                {
                                    if (weird_cjk
                                        /* Special handling of encodings with CJK structure.  */
                                        && ep + 2 <= es
                                        && (unsigned char) ep[0] >= 0x80
                                        && (unsigned char) ep[1] >= 0x30)
                                        {
                                            portion_len += 2;
                                            ep += 1;
                                        }
                                    else
                                        portion_len += 1;
                                }
                        }
                }
            portion = calloc (portion_len, sizeof(*portion));
            overrides = calloc (portion_len, sizeof(*overrides));
            attributes = calloc (portion_len, sizeof(*attributes));
            for (ep = s, pp = portion, op = overrides, ap = attributes; ep < es; ep++)
                {
                    char c = *ep;
                    char attr = 0;
                    char brk = UC_BREAK_UNDEFINED;
                    /* Don't break inside format directives.  */
                    if (attr == ATTR_FORMAT_DIRECTIVE
                        && (fmtdir[ep - value] & FMTDIR_START) == 0)
                        brk = UC_BREAK_PROHIBITED;
                    if (is_escape (c))
                        {
                            switch (c)
                                {
                                case '\a': c = 'a'; break;
                                case '\b': c = 'b'; break;
                                case '\f': c = 'f'; break;
                                case '\n': c = 'n'; break;
                                case '\r': c = 'r'; break;
                                case '\t': c = 't'; break;
                                case '\v': c = 'v'; break;
                                default: abort ();
                                }
                            *pp++ = '\\';
                            *pp++ = c;
                            *op++ = brk;
                            *op++ = UC_BREAK_PROHIBITED;
                            *ap++ = attr | ATTR_ESCAPE_SEQUENCE;
                            *ap++ = attr | ATTR_ESCAPE_SEQUENCE;
                            /* We warn about any use of escape sequences beside
                               '\n' and '\t'.  */
                            if (c != 'n' && c != 't')
                                printf("internationalized messages should not contain the '\\%c' escape sequence\n", c);
                        }
                    else if (escape && !isprint ((unsigned char) c))
                        {
                            *pp++ = '\\';
                            *pp++ = '0' + (((unsigned char) c >> 6) & 7);
                            *pp++ = '0' + (((unsigned char) c >> 3) & 7);
                            *pp++ = '0' + ((unsigned char) c & 7);
                            *op++ = brk;
                            *op++ = UC_BREAK_PROHIBITED;
                            *op++ = UC_BREAK_PROHIBITED;
                            *op++ = UC_BREAK_PROHIBITED;
                            *ap++ = attr | ATTR_ESCAPE_SEQUENCE;
                            *ap++ = attr | ATTR_ESCAPE_SEQUENCE;
                            *ap++ = attr | ATTR_ESCAPE_SEQUENCE;
                            *ap++ = attr | ATTR_ESCAPE_SEQUENCE;
                        }
                    else if (c == '\\' || c == '"')
                        {
                            *pp++ = '\\';
                            *pp++ = c;
                            *op++ = brk;
                            *op++ = UC_BREAK_PROHIBITED;
                            *ap++ = attr | ATTR_ESCAPE_SEQUENCE;
                            *ap++ = attr | ATTR_ESCAPE_SEQUENCE;
                        }
                    else
                        {
#if HAVE_ICONV
                            if (conv != (iconv_t)(-1))
                                {
                                    /* Copy a complete multi-byte character.  Don't
                                       interpret the second byte of a multi-byte character as
                                       ASCII.  This is needed for the BIG5, BIG5-HKSCS, GBK,
                                       GB18030, SHIFT_JIS, JOHAB encodings.  */
                                    char scratchbuf[64];
                                    const char *inptr = ep;
                                    size_t insize;
                                    char *outptr = &scratchbuf[0];
                                    size_t outsize = sizeof (scratchbuf);
                                    size_t res;

                                    res = (size_t)(-1);
                                    for (insize = 1; inptr + insize <= es; insize++)
                                        {
                                            res = iconv (conv,
                                                         (ICONV_CONST char **) &inptr, &insize,
                                                         &outptr, &outsize);
                                            if (!(res == (size_t)(-1) && errno == EINVAL))
                                                break;

                                            /* We expect that no input bytes have been consumed
                                               so far.  */
                                            if (inptr != ep)
                                                abort ();
                                        }
                                    if (res == (size_t)(-1))
                                        {
                                            if (errno == EILSEQ)
                                                {
                                                    printf("invalid multibyte sequence\n");
                                                    continue;
                                                }
                                            else
                                                abort ();
                                        }
                                    insize = inptr - ep;
                                    memcpy (pp, ep, insize);
                                    pp += insize;
                                    *op = brk;
                                    memset (op + 1, UC_BREAK_PROHIBITED, insize - 1);
                                    op += insize;
                                    memset (ap, attr, insize);
                                    ap += insize;
                                    ep += insize - 1;
                                }
                            else
#endif
                                {
                                    if (weird_cjk
                                        /* Special handling of encodings with CJK structure.  */
                                        && ep + 2 <= es
                                        && (unsigned char) c >= 0x80
                                        && (unsigned char) ep[1] >= 0x30)
                                        {
                                            *pp++ = c;
                                            ep += 1;
                                            *pp++ = *ep;
                                            *op++ = brk;
                                            *op++ = UC_BREAK_PROHIBITED;
                                            *ap++ = attr;
                                            *ap++ = attr;
                                        }
                                    else
                                        {
                                            *pp++ = c;
                                            *op++ = brk;
                                            *ap++ = attr;
                                        }
                                }
                        }
                }

            /* Don't break immediately before the "\n" at the end.  */
            if (es > s && es[-1] == '\n')
                overrides[portion_len - 2] = UC_BREAK_PROHIBITED;

            linebreaks = calloc (portion_len, sizeof(*linebreaks));

            /* Subsequent lines after a break are all indented.
               See INDENT-S.  */
            startcol_after_break = (line_prefix ? strlen (line_prefix) : 0);
            if (indent)
                startcol_after_break = (startcol_after_break + extra_indent + 8) & ~7;
            startcol_after_break++;

            /* The line width.  Allow room for the closing quote character.  */
            width = (wrap_strings && do_wrap ? page_width : INT_MAX) - 1;
            /* Adjust for indentation of subsequent lines.  */
            width -= startcol_after_break;

        recompute:
            /* The line starts with different things depending on whether it
               is the first line, and if we are using the indented style.
               See INDENT-F.  */
            startcol = (line_prefix ? strlen (line_prefix) : 0);
            if (first_line)
                {
                    startcol += strlen (name);
                    if (indent)
                        startcol = (startcol + extra_indent + 8) & ~7;
                    else
                        startcol++;
                }
            else
                {
                    if (indent)
                        startcol = (startcol + extra_indent + 8) & ~7;
                }
            /* Allow room for the opening quote character.  */
            startcol++;
            /* Adjust for indentation of subsequent lines.  */
            startcol -= startcol_after_break;

            /* Do line breaking on the portion.  */
            ulc_width_linebreaks (portion, portion_len, width, startcol, 0,
                                  overrides, canon_charset, linebreaks);

            /* If this is the first line, and we are not using the indented
               style, and the line would wrap, then use an empty first line
               and restart.  */
            if (first_line && !indent
                && portion_len > 0
                && (*es != '\0'
                    || startcol > width
                    || memchr (linebreaks, UC_BREAK_POSSIBLE, portion_len) != NULL))
                {
                    if (line_prefix != NULL)
                        buffer_append(out, line_prefix);
                    buffer_append(out, name);
                    buffer_append(out, " ");
                    buffer_append(out, "\"\"");
                    buffer_append(out, "\n");
                    first_line = 0;
                    /* Recompute startcol and linebreaks.  */
                    goto recompute;
                }

            /* Print the beginning of the line.  This will depend on whether
               this is the first line, and if the indented style is being
               used.  INDENT-F.  */
            {
                int currcol = 0;
                char *spacebuffer;
                spacebuffer = malloc(extra_indent + 1);
                if (line_prefix != NULL)
                    {
                        buffer_append(out, line_prefix);
                        currcol = strlen (line_prefix);
                    }
                if (first_line)
                    {
                        buffer_append(out,name);
                        currcol += strlen (name);
                        if (indent)
                            {

                                if (extra_indent > 0) {
                                    snprintf(spacebuffer, extra_indent, "%*s", extra_indent, "");
                                    buffer_append(out, spacebuffer);
                                }
                                currcol += extra_indent;
                                snprintf(spacebuffer, extra_indent, "%*s", 8 - (currcol & 7), "");
                                buffer_append(out, spacebuffer);
                                currcol = (currcol + 8) & ~7;
                            }
                        else
                            {
                                buffer_append(out," ");
                                currcol++;
                            }
                        first_line = 0;
                    }
                else
                    {
                        if (indent)
                            {
                                if (extra_indent > 0) {
                                    snprintf(spacebuffer, extra_indent, "%*s", extra_indent, "");
                                    buffer_append(out, spacebuffer);
                                }
                                currcol += extra_indent;
                                snprintf(spacebuffer, extra_indent, "%*s", 8 - (currcol & 7), "");
                                buffer_append(out, spacebuffer);
                                currcol = (currcol + 8) & ~7;
                            }
                    }
            }

            /* Print the portion itself, with linebreaks where necessary.  */
            {
                char currattr = 0;

                buffer_append(out,"\"");
                for (i = 0; i < portion_len; i++)
                    {
                        if (linebreaks[i] == UC_BREAK_POSSIBLE)
                            {
                                int currcol;

                                /* Change currattr so that it becomes 0.  */
                                if (currattr & ATTR_ESCAPE_SEQUENCE)
                                    {
                                        currattr &= ~ATTR_ESCAPE_SEQUENCE;
                                    }
                                if (currattr & ATTR_FORMAT_DIRECTIVE)
                                    {
                                        currattr &= ~ATTR_FORMAT_DIRECTIVE;
                                    }
                                else if (currattr & ATTR_INVALID_FORMAT_DIRECTIVE)
                                    {
                                        currattr &= ~ATTR_INVALID_FORMAT_DIRECTIVE;
                                    }
                                if (!(currattr == 0))
                                    abort ();

                                buffer_append(out,"\"");
                                buffer_append(out,"\n");
                                currcol = 0;
                                /* INDENT-S.  */
                                if (line_prefix != NULL)
                                    {
                                        buffer_append(out,line_prefix);
                                        currcol = strlen (line_prefix);
                                    }
                                if (indent)
                                    {
                                        buffer_append(out, "        ");
                                        currcol = (currcol + 8) & ~7;
                                    }
                                buffer_append(out,"\"");
                            }
                        /* Change currattr so that it matches attributes[i].  */
                        if (attributes[i] != currattr)
                            {
                                /* class_escape_sequence occurs inside class_format_directive
                                   and class_invalid_format_directive, so clear it first.  */
                                if (currattr & ATTR_ESCAPE_SEQUENCE)
                                    {
                                        currattr &= ~ATTR_ESCAPE_SEQUENCE;
                                    }
                                if (~attributes[i] & currattr & ATTR_FORMAT_DIRECTIVE)
                                    {
                                        currattr &= ~ATTR_FORMAT_DIRECTIVE;
                                    }
                                else if (~attributes[i] & currattr & ATTR_INVALID_FORMAT_DIRECTIVE)
                                    {
                                        currattr &= ~ATTR_INVALID_FORMAT_DIRECTIVE;
                                    }
                                if (attributes[i] & ~currattr & ATTR_FORMAT_DIRECTIVE)
                                    {
                                        currattr |= ATTR_FORMAT_DIRECTIVE;
                                    }
                                else if (attributes[i] & ~currattr & ATTR_INVALID_FORMAT_DIRECTIVE)
                                    {
                                        currattr |= ATTR_INVALID_FORMAT_DIRECTIVE;
                                    }
                                /* class_escape_sequence occurs inside class_format_directive
                                   and class_invalid_format_directive, so set it last.  */
                                if (attributes[i] & ~currattr & ATTR_ESCAPE_SEQUENCE)
                                    {
                                        currattr |= ATTR_ESCAPE_SEQUENCE;
                                    }
                            }
                        buffer_append_char(out, portion[i]); /* WAS             ostream_write_mem (stream, &portion[i], 1);*/
                    }

                /* Change currattr so that it becomes 0.  */
                if (currattr & ATTR_ESCAPE_SEQUENCE)
                    {
                        currattr &= ~ATTR_ESCAPE_SEQUENCE;
                    }
                if (currattr & ATTR_FORMAT_DIRECTIVE)
                    {
                        currattr &= ~ATTR_FORMAT_DIRECTIVE;
                    }
                else if (currattr & ATTR_INVALID_FORMAT_DIRECTIVE)
                    {
                        currattr &= ~ATTR_INVALID_FORMAT_DIRECTIVE;
                    }
                if (!(currattr == 0))
                    abort ();

                buffer_append(out,"\"");
                buffer_append(out,"\n");
            }

            free (linebreaks);
            free (attributes);
            free (overrides);
            free (portion);

            s = es;
#     undef is_escape
        }
    while (*s);

    if (fmtdir != NULL)
        free (fmtdir);

#if HAVE_ICONV
    if (conv != (iconv_t)(-1))
        iconv_close (conv);
#endif
    free(out);
    return output_string;
}

int main(int ac, char **av)
{
    if (ac != 3) {
        printf("Usage: %s [msgid|msgstr] STRING\n", av[0]);
        exit(EXIT_FAILURE);
    }
    printf("%s\n", wrap (NULL, 0, av[1], av[2], 1, 79, "UTF-8"));

}
