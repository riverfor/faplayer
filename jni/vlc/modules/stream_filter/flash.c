
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
    set_capability("stream_filter", 10)
    set_callbacks(Open, Close)
vlc_module_end()

/*****************************************************************************
 *
 *****************************************************************************/

struct stream_sys_t
{
    vlc_url_t     url;
    int           site;
    /* temp */
    int           depth;
    vlc_array_t   *tag;
    int           temp_order;
    char          *temp_url;
    int           temp_length;
    /* final */
    vlc_array_t   *video;
    uint64_t      length; /* total length in ms */
    vlc_thread_t  thread;
};

typedef struct _suburl_t
{
    int order;
    char *url;
    uint64_t start; /* start time indicated by playlist */
    uint64_t length; /* length indicated by playlist */
    stream_t *p_stream;
    uint64_t stream_start;
    uint64_t stream_length;
} suburl_t;

#pragma pack(push)
#pragma pack(1)
typedef struct _FlvHeader_t
{
    uint8_t magic[3];
    uint8_t version;
    uint8_t flag;
    uint32_t data_offset;
} FlvHeader_t;

typedef struct _FlvSegment_t
{
    uint32_t previous_size;
    uint8_t flag;
    uint8_t data_size[3];
    uint8_t timestamp[3];
    uint8_t timestamp_extended;
} FlvSegment_t;
#pragma pack(pop)

/****************************************************************************
 * Local prototypes
 ****************************************************************************/

static bool Identify(stream_t *);

static int  Read   (stream_t *, void *p_read, unsigned int i_read);
static int  Peek   (stream_t *, const uint8_t **pp_peek, unsigned int i_peek);
static int  Control(stream_t *, int i_query, va_list);

/****************************************************************************
 *
 ****************************************************************************/

#define SITE_UNKNOWN 0
#define SITE_SINA 1

static char *s_psz_site_name[] = {
    "unknown",
    "sina"
};

static int parse_int(char *s, int len)
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
    suburl_t *sua = a, *sub = b;
    return (sua->order - sub->order);
}

static void XMLCALL xml_start_element_handler(void *user, XML_Char *name, XML_Char **atts)
{
    stream_t *s = user;
    stream_sys_t *p_sys = s->p_sys;

    // msg_Dbg(s, "start %d %s", p_sys->depth, name);
    vlc_array_append(p_sys->tag, strdup(name));
    p_sys->depth += 1;
}

static void XMLCALL xml_end_element_handler(void *user, XML_Char *name)
{
    stream_t *s = user;
    stream_sys_t *p_sys = s->p_sys;

    // msg_Dbg(s, "end %d %s", p_sys->depth, name);

    if (p_sys->site == SITE_SINA)
    {
        if (p_sys->depth == 2 && !strcmp(name, "durl"))
        {
            suburl_t *su = calloc(1, sizeof(suburl_t));
            su->order = p_sys->temp_order;
            su->url = p_sys->temp_url;
            su->length = p_sys->temp_length;
            vlc_array_append(p_sys->video, su);
        }
    }

    p_sys->depth -= 1;
    char *tag = vlc_array_item_at_index(p_sys->tag, p_sys->depth);
    free(tag);
    vlc_array_remove(p_sys->tag, p_sys->depth);
}

static void XMLCALL xml_character_data_handler(void *user, const XML_Char *str, int len)
{
    stream_t *s = user;
    stream_sys_t *p_sys = s->p_sys;

    char *tag = vlc_array_item_at_index(p_sys->tag, p_sys->depth - 1);
    // msg_Dbg(s, "%d %s %d", p_sys->depth, tag, len);

    if (p_sys->site == SITE_SINA)
    {
        if (p_sys->depth == 2 && !strcmp(tag, "timelength"))
        {
            p_sys->length = parse_int(str, len);
        }
        else if (p_sys->depth == 3 && !strcmp(tag, "order"))
        {
            p_sys->temp_order = parse_int(str, len);
        }
        else if (p_sys->depth == 3 && !strcmp(tag, "url"))
        {
            p_sys->temp_url = strndup(str, len);
        }
        else if (p_sys->depth == 3 && !strcmp(tag, "length"))
        {
            p_sys->temp_length = parse_int(str, len);
        }
    }
}

static bool Identify(stream_t *s)
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
    /* guess */
    p_sys->site = 0;
    if (!strncmp(s->psz_path, "v.iask.com", 10))
        p_sys->site = SITE_SINA;
    /* parse the xml */
    p_sys->tag = vlc_array_new();
    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, s);
    XML_SetStartElementHandler(parser, xml_start_element_handler);
    XML_SetEndElementHandler(parser, xml_end_element_handler);
    XML_SetCharacterDataHandler(parser, xml_character_data_handler);
    int status = XML_Parse(parser, xml, xml_length, 1);
    if (status == XML_STATUS_ERROR)
        msg_Err(s, "%d: %s", XML_GetCurrentLineNumber(parser), XML_ErrorString(XML_GetErrorCode(parser)));
    XML_ParserFree(parser);
    int i = 0;
    for (i = 0; i < vlc_array_count(p_sys->tag); i++)
        free(vlc_array_item_at_index(p_sys->tag, i));
    vlc_array_destroy(p_sys->tag);
    p_sys->tag = NULL;
    free(xml);
    /* reorder if needed */
    if (status == XML_STATUS_OK)
    {
        if (p_sys->site = SITE_SINA)
            qsort(p_sys->video->pp_elems, vlc_array_count(p_sys->video), sizeof(p_sys->video->pp_elems[0]), compare_sina_order);
    }

    return (status == XML_STATUS_OK);
}

static int Open(vlc_object_t *p_this)
{
    stream_t *s = (stream_t *) p_this;
    stream_sys_t *p_sys;

    /* must be http */
    if (strncmp(s->psz_access, "http", 4))
        return VLC_EGENERIC;
    p_sys = calloc(1, sizeof(stream_sys_t));
    if (!p_sys)
        return VLC_ENOMEM;
    s->p_sys = p_sys;
    p_sys->video = vlc_array_new();
    /* try to parse the info */
    if (!Identify(s))
    {
        vlc_array_destroy(p_sys->video);
        free(p_sys);
        msg_Dbg(s, "could not handle %s://%s", s->psz_access, s->psz_path);
        return VLC_EGENERIC;
    }
    msg_Dbg(s, "detected site: %s, length = %d", s_psz_site_name[p_sys->site], p_sys->length);
    int i;
    for (i = 0; i < vlc_array_count(p_sys->video); i++)
    {
        suburl_t *su = vlc_array_item_at_index(p_sys->video, i);
        msg_Dbg(p_this, "    %d %d %s", su->order, su->length, su->url);
    }

    return VLC_EGENERIC;
}

static void Close(vlc_object_t *p_this)
{
}

