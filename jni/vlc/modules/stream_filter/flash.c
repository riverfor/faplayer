
/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <limits.h>
#include <errno.h>

#include <vlc_common.h>
#include <vlc_plugin.h>

#include <assert.h>

#include <vlc_threads.h>
#include <vlc_arrays.h>
#include <vlc_stream.h>
#include <vlc_url.h>
#include <vlc_memory.h>

#include <expat.h>

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int  Open (vlc_object_t *);
static void Close(vlc_object_t *);

vlc_module_begin()
    set_category(CAT_INPUT)
    set_subcategory(SUBCAT_INPUT_STREAM_FILTER)
    set_description(N_("Multi-parted flash video stream filter"))
    set_capability("stream_filter", 1)
    set_callbacks(Open, Close)
vlc_module_end()

/*****************************************************************************
 *
 *****************************************************************************/

typedef struct _suburl_t
{
    int i_order; /* order */
    char *psz_url; /* url */
    uint32_t i_start; /* start time indicated by playlist */
    uint32_t i_length; /* length indicated by playlist */
    uint64_t i_offset; /* = sum of the size of previous streams */
    stream_t *p_stream; /* this stream */
    uint64_t i_stream_skip; /* flv header and metadata size */
    uint64_t i_stream_size; /* real stream size */
} suburl_t;

#define HACK_TYPE_SIZE 1
#define HACK_TYPE_TIME 2

typedef struct _flv_tag_t
{
    int64_t i_offset;
    int i_size;
    uint8_t *p_data;
    uint8_t *p_read;
    int i_hack;
    uint8_t p_hack_data[4];
} flv_tag_t;

#pragma pack(push)
#pragma pack(1)
typedef struct _flv_header_t
{
    uint8_t magic[3];
    uint8_t version;
    uint8_t flag;
    uint8_t data_offset[4];
} flv_header_t;

typedef struct _flv_tag_header_t
{
    uint8_t previous_size[4];
    uint8_t flag;
    uint8_t data_size[3];
    uint8_t timestamp[3];
    uint8_t timestamp_extended;
    uint8_t stream_id[3];
} flv_tag_header_t;
#pragma pack(pop)

// XXX: this is little endian

#define FLV_UI24(x) ((((uint8_t *) x)[0] << 16) | (((uint8_t *) x)[1] << 8) | (((uint8_t *) x)[2]))
#define FLV_UI32(x) ((((uint8_t *) x)[0] << 24) | (((uint8_t *) x)[1] << 16) | (((uint8_t *) x)[2] << 8) | (((uint8_t *) x)[3]))

#define FLV_PUT_UI24(d, x) (((uint8_t *) d)[0] = (x & 0xff0000) >> 16; ((uint8_t *) d)[1] = (x & 0xff00) >> 8; ((uint8_t *) d)[0] = (x & 0xff);)
#define FLV_PUT_UI32(d, x) *((uint32_t *) d) = (((x & 0xff000000) >> 24) | ((x & 0xff0000) >> 8) | ((x & 0xff00) << 8) | ((x & 0xff) << 24));

struct stream_sys_t
{
    /* used for parsing playlist */
    int           temp_depth;
    vlc_array_t   *temp_tag;
    int           temp_order;
    char          *temp_url;
    int           temp_length;
    /* */
    int           i_site;
    vlc_array_t   *p_video;
    uint32_t      i_length; /* total length indicated by playlist */
    uint64_t      i_offset; /* virtual stream offset */
    uint64_t      i_size;   /* virtual stream size */
    uint64_t      i_flv_offset; /* current read offset */
    uint64_t      i_flv_parsed_bytes; /* how many bytes we have parsed */
    vlc_array_t   *p_flv_tag;
    /* */
    int i_peek;
    void *p_peek;
};

/****************************************************************************
 * Local prototypes
 ****************************************************************************/

static bool DetectStream(stream_t *);

static int LoadStream(stream_t *);
static void FreeStream(stream_t *);

static int  Read   (stream_t *, void *p_read, unsigned int i_read);
static int  Peek   (stream_t *, const uint8_t **pp_peek, unsigned int i_peek);
static int  Control(stream_t *, int i_query, va_list);

static int FlvCheckHeader(const uint8_t *p_data);
static void FlvSetInt(void *p_tag, const char *psz_name, int i_value);

/****************************************************************************
 *
 ****************************************************************************/

#define SITE_NONE       0
#define SITE_SINA       1

static char *s_psz_site_name[] = {
    "none",
    "sina"
};

static int parse_int(const char *s, int len)
{
    int i, val = 0;

    for (i = 0; i < len; i++)
    {
        char c = s[i];
        val = val * 10 + (c - '0');
    }

    return val;
}

static int compare_sina_order(const void *a, const void *b)
{
    suburl_t *sua = (suburl_t *) a, *sub = (suburl_t *) b;
    return (sua->i_order - sub->i_order);
}

static void XMLCALL xml_start_element_handler(void *user, XML_Char *name, XML_Char **atts)
{
    stream_t *s = user;
    stream_sys_t *p_sys = s->p_sys;

    vlc_array_append(p_sys->temp_tag, strdup(name));
    p_sys->temp_depth += 1;
}

static void XMLCALL xml_end_element_handler(void *user, XML_Char *name)
{
    stream_t *s = user;
    stream_sys_t *p_sys = s->p_sys;

    if (p_sys->i_site == SITE_SINA)
    {
        if (p_sys->temp_depth == 2 && !strcmp(name, "durl"))
        {
            suburl_t *su = calloc(1, sizeof(suburl_t));
            su->i_order = p_sys->temp_order;
            su->psz_url = p_sys->temp_url;
            su->i_length = p_sys->temp_length;
            vlc_array_append(p_sys->p_video, su);
        }
    }

    p_sys->temp_depth -= 1;
    char *tag = vlc_array_item_at_index(p_sys->temp_tag, p_sys->temp_depth);
    free(tag);
    vlc_array_remove(p_sys->temp_tag, p_sys->temp_depth);
}

static void XMLCALL xml_character_data_handler(void *user, const XML_Char *str, int len)
{
    stream_t *s = user;
    stream_sys_t *p_sys = s->p_sys;

    char *tag = vlc_array_item_at_index(p_sys->temp_tag, p_sys->temp_depth - 1);

    if (p_sys->i_site == SITE_SINA)
    {
        if (p_sys->temp_depth == 2 && !strcmp(tag, "timelength"))
        {
            p_sys->i_length = parse_int(str, len);
        }
        else if (p_sys->temp_depth == 3 && !strcmp(tag, "order"))
        {
            p_sys->temp_order = parse_int(str, len);
        }
        else if (p_sys->temp_depth == 3 && !strcmp(tag, "url"))
        {
            p_sys->temp_url = strndup(str, len);
        }
        else if (p_sys->temp_depth == 3 && !strcmp(tag, "length"))
        {
            p_sys->temp_length = parse_int(str, len);
        }
    }
}

static int GetStreamIndex(stream_t *s, int64_t position)
{
    stream_sys_t *p_sys = s->p_sys;
    suburl_t *su;
    int i;
    int min = 0, max = vlc_array_count(p_sys->p_video);
    if (max == 0)
        return -1;
    else if (max > 2)
    {
        i = (min + max) / 2;
        while (min != max)
        {
            su = (suburl_t *) vlc_array_item_at_index(p_sys->p_video, i);
            if (position < su->i_offset)
                max = i;
            else if (position >= (su->i_offset + su->i_stream_size - su->i_stream_skip))
                min = i + 1;
            else
                break;
            i = (min + max) / 2;
        }
    }
    else
    {
        for (i = 0; i < max; i++)
        {
            if ((position >= su->i_offset) && (position < (su->i_offset + su->i_stream_size - su->i_stream_skip)))
                break;
        }
    }
    return (i == max) ? -1 : i;
}

static int GetFlvTagIndex(stream_t *s, int64_t position)
{
    stream_sys_t *p_sys = s->p_sys;
    flv_tag_t *tag;
    int i;
    int min = 0, max = vlc_array_count(p_sys->p_flv_tag);
    if (max == 0)
        return -1;
    i = (min + max) / 2;
    while (min != max)
    {
        tag = (flv_tag_t *) vlc_array_item_at_index(p_sys->p_flv_tag, i);
        if (position < tag->i_offset)
            max = i;
        else if (position >= (tag->i_offset + tag->i_size))
            min = i + 1;
        else
            break;
        i = (min + max) / 2;
    }
    return (i == max) ? -1 : i;
}

static int EnsureFlvTagReady(stream_t *s, int index)
{
    stream_sys_t *p_sys = s->p_sys;

    flv_tag_t *tag = vlc_array_item_at_index(p_sys->p_flv_tag, index);
    if (!tag)
        return VLC_EGENERIC;
    flv_tag_t *previous_tag = vlc_array_item_at_index(p_sys->p_flv_tag, index - 1);
    if (!tag->p_data)
    {
        /* read a flv tag */
        tag->p_data = malloc(tag->i_size);
        if (!tag->p_data)
            return VLC_ENOMEM;
        int i_stream = GetStreamIndex(s, tag->i_offset);
        suburl_t *su = vlc_array_item_at_index(p_sys->p_video, i_stream);
        int n = stream_Read(su->p_stream, tag->p_data, tag->i_size);
        if (n != tag->i_size)
        {
            free(tag->p_data);
            tag->p_data = NULL;
            return VLC_EGENERIC;
        }
        tag->p_read = tag->p_data;
        /* "correct" fields if any */
        flv_tag_header_t *tag_header = (flv_tag_header_t *) tag->p_data;
        if (tag->i_hack & HACK_TYPE_TIME)
        {
            uint32_t val = FLV_UI32(tag_header->timestamp) + su->i_start;
            FLV_PUT_UI32(tag_header->timestamp, val);
        }
        if (tag->i_hack & HACK_TYPE_SIZE)
        {
            uint32_t val = previous_tag ? previous_tag->i_size : 0;
            FLV_PUT_UI32(tag_header->previous_size, val);
        }
    }
    return VLC_SUCCESS;
}

static int GetNextFlvTag(stream_t *s)
{
    stream_sys_t *p_sys = s->p_sys;

    /* determine which stream to read */
    int i_stream = GetStreamIndex(s, p_sys->i_flv_offset);
    suburl_t *su = vlc_array_item_at_index(p_sys->p_video, i_stream);
    if (!su)
        return VLC_EGENERIC;
    int i_hack = 0;
    if (i_stream > 0)
    {
        i_hack |= HACK_TYPE_TIME;
        if (su->i_offset == p_sys->i_flv_offset)
            i_hack |= HACK_TYPE_SIZE;
    }
    const uint8_t *peek;
    int peek_size = stream_Peek(su->p_stream, &peek, sizeof(flv_tag_header_t));
    if (peek_size != sizeof(flv_tag_header_t))
        return VLC_EGENERIC;
    /* try to find the flv tag */
    flv_tag_t *p_tag = NULL;
    int i_tag = -1;
    for (int n = vlc_array_count(p_sys->p_flv_tag) - 1; n >= 0; n--)
    {
        flv_tag_t *t = (flv_tag_t *) vlc_array_item_at_index(p_sys->p_flv_tag, n);
        if (t->i_offset == p_sys->i_flv_offset)
        {
            p_tag = t;
            i_tag = n;
            break;
        }
    }
    /* allocate a new flv tag */
    if (!p_tag)
    {
        flv_tag_header_t *th = (flv_tag_header_t *) peek;
        int data_size = FLV_UI24(&th->data_size);
        p_tag = calloc(1, sizeof(flv_tag_t));
        if (!p_tag)
            return VLC_ENOMEM;
        p_tag->i_offset = p_sys->i_flv_parsed_bytes;
        p_tag->i_size = sizeof(flv_tag_header_t) + data_size;
        p_sys->i_flv_parsed_bytes += p_tag->i_size;
        vlc_array_append(p_sys->p_flv_tag, p_tag);
        i_tag = vlc_array_count(p_sys->p_flv_tag) - 1;
    }
    /* */
    return EnsureFlvTagReady(s, i_tag);
}

static int ReadStream(stream_t *s, void *p_read, unsigned int i_read)
{
    stream_sys_t *p_sys = s->p_sys;

    unsigned int i_copy = 0;
    while (i_copy < i_read)
    {
        break;
    }

    return i_copy;
}

static int PeekStream(stream_t *s, const uint8_t **pp_peek, unsigned int i_peek)
{
    stream_sys_t *p_sys = s->p_sys;

    /* prepare space for peeking */
    if (p_sys->i_peek < i_peek)
    {
        void *p_peek = realloc(p_sys->p_peek, i_peek);
        if (p_peek == NULL)
            return 0;
        p_sys->p_peek = p_peek;
    }
    /* */
    int i_tag = GetFlvTagIndex(s, p_sys->i_offset);
    while (i_tag < 0)
    {
        int rc = GetNextFlvTag(s);
        if (rc < 0)
        {
            msg_Err(s, "failed to read next flv tag");
            return 0;
        }
        i_tag = GetFlvTagIndex(s, p_sys->i_offset);
    }
    /* */
    unsigned int i_copy = 0;
    while (i_copy < i_peek)
    {
        flv_tag_t *tag = vlc_array_item_at_index(p_sys->p_flv_tag, i_tag);
        if (!tag)
        {
            int rc = GetNextFlvTag(s);
            if (rc < 0)
                break;
            continue;
        }
        if (EnsureFlvTagReady(s, i_tag) < 0)
            break;
        i_tag += 1;
        
        
    }
    *pp_peek = p_sys->p_peek;

    return i_copy;
}

static int SeekStream(stream_t *s, int64_t offset)
{
    stream_sys_t *p_sys = s->p_sys;

    if (offset >= p_sys->i_size)
        return VLC_EGENERIC;
    /* seek */
    int64_t seek_position = (offset <= p_sys->i_flv_parsed_bytes) ? offset : p_sys->i_flv_parsed_bytes;
    suburl_t *su = vlc_array_item_at_index(p_sys->p_video, GetStreamIndex(s, p_sys->i_offset));
    int64_t stream_position = su->i_stream_skip + seek_position - su->i_offset;
    if (stream_Tell(su->p_stream) != stream_position)
    {
        int rc = stream_Seek(su->p_stream, stream_position);
        if (rc < 0)
        {
            msg_Err(s, "failed to seek #%d to %"PRId64, su->i_order, stream_position);
            return VLC_EGENERIC;
        }
    }
    /* read extra bytes */
    int chunk = 65536;
    int64_t count = 0;
    int64_t total = offset - seek_position;
    if (total > 0) {
        while (count < total)
        {
            int temp = ReadStream(s, NULL, chunk);
            count += temp;
            if (temp < chunk)
                break;
        }
        if (count < total)
        {
            msg_Dbg(s, "failed to seek to %"PRId64, offset);
            return VLC_EGENERIC;
        }
    }
    /* looks good */
    p_sys->i_offset = offset;

    return VLC_SUCCESS;
}

static bool DetectStream(stream_t *s)
{
    const uint8_t *peek;
    char *xml;
    int xml_length;
    stream_sys_t *p_sys = s->p_sys;

    /* should be enough to hold all contents */
    int64_t peek_size = stream_Peek(s->p_source, &peek, 65536);
    if (peek_size < 1)
        return false;
    if (strncmp("<?xml", peek, 5))
        return false;
    /* parse begin tag */
    char *encoding = NULL, *encoding_start, *encoding_end;
    encoding_start = strstr(peek, "encoding=\"");
    if (encoding_start)
    {
        encoding_start += 10;
        encoding_end = strchr(encoding_start, '\"');
        int encoding_length = encoding_end - encoding_start;
        if (encoding_length > 0)
        {
            encoding = malloc(encoding_length + 1);
            strncpy(encoding, encoding_start, encoding_length);
            encoding[encoding_length] = '\0';
        }
    }
    /* convert non utf-8 to utf-8 */
    if (encoding != NULL && strcasecmp(encoding, "UTF-8"))
    {
        // TODO:
    }
    else
    {
        xml = malloc(peek_size + 1);
        memcpy(xml, peek, peek_size);
        xml[peek_size] = 0;
        xml_length = peek_size;
    }
    if (encoding)
        free(encoding);
    /* guess type from URL */
    p_sys->i_site = SITE_NONE;
    if (!strncmp(s->psz_path, "v.iask.com", 10))
        p_sys->i_site = SITE_SINA;
    if (p_sys->i_site == SITE_NONE)
        return false;
    /* parse the XML formatted playlist */
    p_sys->p_video = vlc_array_new();
    p_sys->temp_tag = vlc_array_new();
    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, s);
    XML_SetStartElementHandler(parser, (XML_StartElementHandler) xml_start_element_handler);
    XML_SetEndElementHandler(parser, (XML_EndElementHandler) xml_end_element_handler);
    XML_SetCharacterDataHandler(parser, xml_character_data_handler);
    int i = 0;
    int status = XML_Parse(parser, xml, xml_length, 1);
    /* failed to parse */
    if (status == XML_STATUS_ERROR)
    {
        /* release any result */
        for (i = 0; i < vlc_array_count(p_sys->p_video); i++)
        {
            suburl_t *su = vlc_array_item_at_index(p_sys->p_video, i);
            if (su->psz_url)
                free(su->psz_url);
            free(su);
        }
        vlc_array_destroy(p_sys->p_video);
        msg_Err(s, "Error parsing XML at %d: %s", XML_GetCurrentLineNumber(parser), XML_ErrorString(XML_GetErrorCode(parser)));
    }
    XML_ParserFree(parser);
    for (i = 0; i < vlc_array_count(p_sys->temp_tag); i++)
        free(vlc_array_item_at_index(p_sys->temp_tag, i));
    vlc_array_destroy(p_sys->temp_tag);
    free(xml);
    /* misc */
    if (status == XML_STATUS_OK)
    {
        /* for sina url */
        if (p_sys->i_site = SITE_SINA)
        {
            qsort(p_sys->p_video->pp_elems, vlc_array_count(p_sys->p_video), sizeof(p_sys->p_video->pp_elems[0]), compare_sina_order);
            /* calculate time */
            for (i = 1; i < vlc_array_count(p_sys->p_video); i++)
            {
                suburl_t *su0 = vlc_array_item_at_index(p_sys->p_video, i - 1);
                suburl_t *su1 = vlc_array_item_at_index(p_sys->p_video, i);
                su1->i_start = su0->i_start + su0->i_length;
            }
        }
    }

    return (status == XML_STATUS_OK);
}

static int LoadStream(stream_t *s)
{
    stream_sys_t *p_sys = s->p_sys;
    p_sys->i_size = 0;
    p_sys->i_offset = 0;
    int i, n = vlc_array_count(p_sys->p_video);
    if (n == 0)
        return VLC_EGENERIC;
    for (i = 0; i < n; i++)
    {
        /* try to open the URL */
        suburl_t *su = vlc_array_item_at_index(p_sys->p_video, i);
        stream_t *p_stream = stream_UrlNew(s, su->psz_url);
        if (p_stream == NULL)
        {
            msg_Err(s, "failed to open %s", su->psz_url);
            return VLC_EGENERIC;
        }
        su->p_stream = p_stream;
        su->i_stream_size = stream_Size(p_stream);
        /* try to read flv header and the first tag */
        const uint8_t *peek;
        uint64_t peek_size = stream_Peek(p_stream, &peek, sizeof(flv_header_t) + sizeof(flv_tag_header_t));
        if (peek_size != sizeof(flv_header_t) + sizeof(flv_tag_header_t))
        {
            msg_Err(s, "failed to read %s", su->psz_url);
            return VLC_EGENERIC;
        }
        if (FlvCheckHeader(peek) < 0)
        {
            msg_Err(s, "#%d does not look like flv formatted", i);
            return VLC_EGENERIC;
        }
        p_sys->i_size += su->i_stream_size;
        /* flv header */
        flv_header_t *header = (flv_header_t *) peek;
        /* the first tag of flv */
        flv_tag_header_t *tag_header = (flv_tag_header_t *)(peek + sizeof(flv_tag_header_t));
        uint8_t first_flag = tag_header->flag;
        /* whether skip flv header */
        su->i_stream_skip = (i > 0) ? FLV_UI32(header->data_offset) : 0;
        /* whether skip metadata */
        if ((first_flag & 0x12) == 0x12)
        {
            if (i > 0)
            {
                su->i_stream_skip += sizeof(flv_tag_header_t);
                su->i_stream_skip += FLV_UI24(tag_header->data_size);
                p_sys->i_size -= su->i_stream_skip;
                suburl_t *prev = (suburl_t *) vlc_array_item_at_index(p_sys->p_video, i - 1);
                su->i_offset += (prev->i_offset + prev->i_stream_size - prev->i_stream_skip);
                msg_Dbg(s, "#%d will begin at offset %"PRId64" bytes", su->i_order, su->i_stream_skip);
            }
            else
            {
                su->i_offset = 0;
            }
        }
    }
    p_sys->p_flv_tag = vlc_array_new();
    /* hold flv header */
    flv_tag_t *header_tag = calloc(1, sizeof(flv_tag_t));
    if (!header_tag)
        return VLC_ENOMEM;
    header_tag->i_offset = 0;
    header_tag->i_size = sizeof(flv_header_t);
    header_tag->p_data = calloc(1, sizeof(flv_header_t));
    if (!header_tag->p_data)
    {
        free(header_tag);
        return VLC_ENOMEM;
    }
    header_tag->p_read = header_tag->p_data;
    flv_header_t *header_flv = (flv_header_t *) header_tag->p_data;
    suburl_t *first = vlc_array_item_at_index(p_sys->p_video, 0);
    int rd = stream_Read(first->p_stream, header_tag->p_data, header_tag->i_size);
    if (rd != header_tag->i_size)
    {
        free(header_tag->p_data);
        free(header_tag);
        return VLC_EGENERIC;
    }
    vlc_array_append(p_sys->p_flv_tag, header_tag);
    p_sys->i_flv_offset = sizeof(flv_header_t);
    p_sys->i_flv_parsed_bytes = sizeof(flv_header_t);
    /* hold flv metadata */
    if (GetNextFlvTag(s) < 0)
    {
        free(header_tag->p_data);
        free(header_tag);
        return VLC_EGENERIC;
    }
    /* do "fixes" on metadata tag */
    // XXX: TODO

    return VLC_SUCCESS;
}

static void FreeStream(stream_t *s)
{
    stream_sys_t *p_sys = s->p_sys;

    int i;
    for (i = 0; i < vlc_array_count(p_sys->p_video); i++)
    {
        suburl_t *su = vlc_array_item_at_index(p_sys->p_video, i);
        if (su->psz_url)
            free(su->psz_url);
        if (su->p_stream)
            stream_Delete(su->p_stream);
        free(su);
    }
    vlc_array_destroy(p_sys->p_video);
    for (i = 0; i < vlc_array_count(p_sys->p_flv_tag); i++)
    {
        flv_tag_t *tag = vlc_array_item_at_index(p_sys->p_flv_tag, i);
        if (tag)
        {
            if (tag->p_data)
                free(tag->p_data);
            free(tag);
        }
    }
    vlc_array_destroy(p_sys->p_flv_tag);
    if (p_sys->p_peek)
        free(p_sys->p_peek);
    free(p_sys);
}

static int Open(vlc_object_t *p_this)
{
    stream_t *s = (stream_t *) p_this;
    stream_sys_t *p_sys;

    /* must be http? */
    if (strcmp(s->psz_access, "http"))
        return VLC_EGENERIC;
    p_sys = calloc(1, sizeof(stream_sys_t));
    if (!p_sys)
        return VLC_ENOMEM;
    s->p_sys = p_sys;
    /* try to parse the info */
    if (!DetectStream(s))
    {
        free(p_sys);
        msg_Dbg(s, "could not handle %s://%s", s->psz_access, s->psz_path);
        return VLC_EGENERIC;
    }
    msg_Dbg(s, "detected site: %s, length = %"PRId64"", s_psz_site_name[p_sys->i_site], p_sys->i_length);
    int i;
    for (i = 0; i < vlc_array_count(p_sys->p_video); i++)
    {
        suburl_t *su = vlc_array_item_at_index(p_sys->p_video, i);
        msg_Dbg(p_this, "#%d %"PRId64" - %"PRId64" %s", su->i_order, su->i_start, su->i_length, su->psz_url);
    }
    /* try to open the files */
    if (LoadStream(s) < 0)
    {
        FreeStream(s);
        return VLC_EGENERIC;
    }
    /* everything looks good */
    s->pf_read = Read;
    s->pf_peek = Peek;
    s->pf_control = Control;

    return VLC_SUCCESS;
}

static void Close(vlc_object_t *p_this)
{
    FreeStream((stream_t *) p_this);
}

static int Read(stream_t *s, void *p_read, unsigned int i_read)
{
    return ReadStream(s, p_read, i_read);
}

static int Peek(stream_t *s, const uint8_t **pp_peek, unsigned int i_peek)
{
    return PeekStream(s, pp_peek, i_peek);
}

static int Control(stream_t *s, int i_query, va_list args)
{
    stream_sys_t *p_sys = s->p_sys;

    switch (i_query)
    {
        case STREAM_CAN_SEEK:
        {
            *(va_arg (args, bool *)) = true;
            break;
        }
        case STREAM_CAN_FASTSEEK:
        {
            *(va_arg (args, bool *)) = false;
            break;
        }
        case STREAM_GET_POSITION:
        {
            *(va_arg (args, uint64_t *)) = p_sys->i_offset;
            break;
        }
        case STREAM_SET_POSITION:
        {
            uint64_t new_offset = *(va_arg (args, uint64_t *));
            return SeekStream(s, new_offset);
        }
        case STREAM_GET_SIZE:
        {
            *(va_arg (args, uint64_t *)) = p_sys->i_size;
            break;
        }
        default:
            return VLC_EGENERIC;
    }
    return VLC_SUCCESS;
}

static int FlvCheckHeader(const uint8_t *p_data)
{
    return VLC_SUCCESS;
}

static void FlvSetInt(void *p_tag, const char *psz_name, int i_value)
{

}



