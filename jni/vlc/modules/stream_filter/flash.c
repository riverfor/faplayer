
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
#include <vlc_threads.h>
#include <vlc_arrays.h>
#include <vlc_stream.h>
#include <vlc_url.h>
#include <vlc_memory.h>

#include <assert.h>
#include <android/log.h>

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
    uint64_t i_start; /* start time indicated by playlist */
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

#define SCRIPT_DATA_NUMBER       0
#define SCRIPT_DATA_BOOLEAN      1
#define SCRIPT_DATA_STRING       2
#define SCRIPT_DATA_ECMA         8

// XXX: this is little endian

#define FLV_UI16(x) (((uint8_t *) x)[0] << 8) | ((uint8_t *) x)[1]
#define FLV_UI24(x) ((((uint8_t *) x)[0] << 16) | (((uint8_t *) x)[1] << 8) | (((uint8_t *) x)[2]))
#define FLV_UI32(x) ((((uint8_t *) x)[0] << 24) | (((uint8_t *) x)[1] << 16) | (((uint8_t *) x)[2] << 8) | (((uint8_t *) x)[3]))

#define FLV_PUT_UI24(d, x) (((uint8_t *) d)[0] = (x & 0xff0000) >> 16; ((uint8_t *) d)[1] = (x & 0xff00) >> 8; ((uint8_t *) d)[0] = (x & 0xff);)
#define FLV_PUT_UI32(d, x) *((uint32_t *) d) = (((x & 0xff000000) >> 24) | ((x & 0xff0000) >> 8) | ((x & 0xff00) << 8) | ((x & 0xff) << 24));

struct stream_sys_t
{
    vlc_array_t   *p_video;
    uint64_t      i_length; /* total length indicated by playlist */
    uint64_t      i_offset; /* virtual stream offset */
    uint64_t      i_size;   /* virtual stream size */
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

static int GetStreamIndex(stream_t *s, int64_t position);
static int GetFlvTagIndex(stream_t *s, int64_t position);
static int64_t GetStreamParsedPosition(stream_t *s);
static int EnsureStreamParsed(stream_t *s, uint64_t i_offset);
static int EnsureStreamPosition(stream_t *s, uint64_t i_offset);
static int ReadFlvTag(stream_t *s, int index);
static int ReadNextFlvTag(stream_t *s, uint64_t i_offset);

static int FlvCheckHeader(const uint8_t *p_data);
static void FlvMetaDataSetDuration(void *p_tag, int i_tag_size, int64_t i_value);

/****************************************************************************
 *
 ****************************************************************************/

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
            su = (suburl_t *) vlc_array_item_at_index(p_sys->p_video, i);
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

static int64_t GetStreamParsedPosition(stream_t *s)
{
    stream_sys_t *p_sys = s->p_sys;
    int64_t i_parsed = 0;
    int i_count = vlc_array_count(p_sys->p_flv_tag);
    if (i_count > 0)
    {
        flv_tag_t *t = vlc_array_item_at_index(p_sys->p_flv_tag, i_count - 1);
        i_parsed = t->i_offset + t->i_size;
    }
    return i_parsed;
}

static int EnsureStreamParsed(stream_t *s, uint64_t i_offset)
{
    uint64_t i_parsed = GetStreamParsedPosition(s);
    while (i_offset > i_parsed)
    {
        int rc = ReadNextFlvTag(s, i_parsed);
        if (rc < 0)
            return VLC_EGENERIC;
        i_parsed = GetStreamParsedPosition(s);
    }
    return VLC_SUCCESS;
}

static int EnsureStreamPosition(stream_t *s, uint64_t i_offset)
{
    stream_sys_t *p_sys = s->p_sys;
    int i_stream = GetStreamIndex(s, i_offset);
    suburl_t *su = vlc_array_item_at_index(p_sys->p_video, i_stream);
    uint64_t i_pos = i_offset - su->i_offset + su->i_stream_skip;
    if (stream_Seek(su->p_stream, i_pos) < 0)
        return VLC_EGENERIC;
    return VLC_SUCCESS;
}

static int ReadFlvTag(stream_t *s, int index)
{
    stream_sys_t *p_sys = s->p_sys;

    flv_tag_t *tag = vlc_array_item_at_index(p_sys->p_flv_tag, index);
    if (!tag)
        return VLC_EGENERIC;
    flv_tag_t *previous_tag = index > 0 ? vlc_array_item_at_index(p_sys->p_flv_tag, index - 1) : NULL;
    if (!tag->p_data)
    {
        if (EnsureStreamPosition(s, tag->i_offset) < 0)
        {
            msg_Err(s, "Could not seek to virtual offset %"PRId64, tag->i_offset);
            return VLC_EGENERIC;
        }
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
            msg_Err(s, "Could not read #%d for %d bytes", su->i_order, tag->i_size);
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

static int ReadNextFlvTag(stream_t *s, uint64_t i_offset)
{
    stream_sys_t *p_sys = s->p_sys;

    /* determine which stream to read */
    int i_stream = GetStreamIndex(s, i_offset);
    suburl_t *su = vlc_array_item_at_index(p_sys->p_video, i_stream);
    if (!su)
    {
        msg_Err(s, "Offset %"PRId64" is out of range", i_offset);
        return VLC_EGENERIC;
    }
    int i_hack = 0;
    if (i_stream > 0)
    {
        i_hack |= HACK_TYPE_TIME;
        if (su->i_offset == i_offset)
            i_hack |= HACK_TYPE_SIZE;
    }
    if (EnsureStreamPosition(s, i_offset) < 0)
    {
        msg_Err(s, "Could not seek to virtual offset %"PRId64, i_offset);
        return VLC_EGENERIC;
    }
    const uint8_t *peek;
    int peek_size = stream_Peek(su->p_stream, &peek, sizeof(flv_tag_header_t));
    if (peek_size != sizeof(flv_tag_header_t))
    {
        msg_Err(s, "Could not peek #%d for %d bytes at virtual offset %"PRId64, i_stream, sizeof(flv_tag_header_t), i_offset);
        return VLC_EGENERIC;
    }
    /* try to find the flv tag */
    flv_tag_t *p_tag = NULL;
    int i_tag = -1;
    for (int n = vlc_array_count(p_sys->p_flv_tag) - 1; n >= 0; n--)
    {
        flv_tag_t *t = (flv_tag_t *) vlc_array_item_at_index(p_sys->p_flv_tag, n);
        if (t->i_offset == i_offset)
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
        p_tag->i_offset = i_offset;
        p_tag->i_size = sizeof(flv_tag_header_t) + data_size;
        vlc_array_append(p_sys->p_flv_tag, p_tag);
        i_tag = vlc_array_count(p_sys->p_flv_tag) - 1;
    }
    /* */
    return ReadFlvTag(s, i_tag);
}

static bool DetectStream(stream_t *s)
{
    stream_sys_t *p_sys = s->p_sys;
    const uint8_t *p_peek;
    int i_peek = stream_Peek(s->p_source, &p_peek, 4);
    if (i_peek != 4)
        return false;
    if (strncmp(p_peek, "#FLV", 4))
        return false;
    p_sys->p_video = vlc_array_new();
    int i_count = -1;
    char *psz_line;
    while ((psz_line = stream_ReadLine(s->p_source)))
    {
        i_count += 1;
        if (i_count == 0)
            continue;
        suburl_t *su = calloc(1, sizeof(suburl_t));
        char *psz_comma = strchr(psz_line, ',');
        su->i_order = i_count;
        if (su->i_order > 1)
            su->i_start = strtoll(psz_line, &psz_comma, 10);
        else
        {
            su->i_start = 0;
            p_sys->i_length = strtoll(psz_line, &psz_comma, 10);
        }
        su->psz_url = decode_URI_duplicate(psz_comma + 1);
        vlc_array_append(p_sys->p_video, su);
        free(psz_line);
    }
    if (vlc_array_count(p_sys->p_video) == 0)
    {
        vlc_array_destroy(p_sys->p_video);
        return false;
    }
    return true;
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
            msg_Err(s, "Could not open %s", su->psz_url);
            return VLC_EGENERIC;
        }
        su->p_stream = p_stream;
        su->i_stream_size = stream_Size(p_stream);
        /* try to read flv header and the first tag */
        const uint8_t *peek;
        uint64_t peek_size = stream_Peek(p_stream, &peek, sizeof(flv_header_t) + sizeof(flv_tag_header_t));
        if (peek_size != sizeof(flv_header_t) + sizeof(flv_tag_header_t))
        {
            msg_Err(s, "Could not read %s", su->psz_url);
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
            }
            else
            {
                su->i_offset = 0;
            }
        }
        msg_Dbg(s, "#%d will begin at offset %"PRId64" bytes", su->i_order, su->i_stream_skip);
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
    /* hold flv metadata */
    if (ReadNextFlvTag(s, sizeof(flv_header_t)) < 0)
    {
        free(header_tag->p_data);
        free(header_tag);
        return VLC_EGENERIC;
    }
    /* do "fixes" on metadata tag */
    flv_tag_t *metadata_tag = vlc_array_item_at_index(p_sys->p_flv_tag, 1);
    FlvMetaDataSetDuration((uint8_t *) metadata_tag->p_data + sizeof(flv_tag_header_t), metadata_tag->i_size, p_sys->i_length);

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

    p_sys = calloc(1, sizeof(stream_sys_t));
    if (!p_sys)
        return VLC_ENOMEM;
    s->p_sys = p_sys;
    /* try to parse the info */
    if (!DetectStream(s))
    {
        free(p_sys);
        return VLC_EGENERIC;
    }
    int i;
    for (i = 0; i < vlc_array_count(p_sys->p_video); i++)
    {
        suburl_t *su = vlc_array_item_at_index(p_sys->p_video, i);
        msg_Dbg(p_this, "#%d %"PRId64" %s", su->i_order, su->i_start, su->psz_url);
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
    stream_sys_t *p_sys = s->p_sys;

    /* ensure the data is ready to read */
    int64_t i_size = (p_sys->i_offset + i_read > p_sys->i_size) ? p_sys->i_size : (p_sys->i_offset + i_read);
    if (EnsureStreamParsed(s, i_size) < 0)
    {
        msg_Err(s, "Could not read to virtual offset %"PRId64" bytes", p_sys->i_offset);
        return 0;
    }
    /* the real work */
    unsigned int i_copy = 0;
    while (i_copy < i_read)
    {
        int i_tag = GetFlvTagIndex(s, p_sys->i_offset + i_copy);
        flv_tag_t *t = vlc_array_item_at_index(p_sys->p_flv_tag, i_tag);
        if (ReadFlvTag(s, i_tag) < 0)
        {
            msg_Err(s, "Could not read data of tag #%d (offset = %"PRId64", size = %d)", i_tag, t->i_offset, t->i_size);
            break;
        }
        int i_stream = GetStreamIndex(s, p_sys->i_offset + i_copy);
        suburl_t *su = vlc_array_item_at_index(p_sys->p_video, i_stream);
        int i_start = (p_sys->i_offset > t->i_offset) ? (p_sys->i_offset - t->i_offset) : 0;
        int i_count = (i_read - i_copy) < (t->i_size - i_start) ? (i_read - i_copy) : (t->i_size - i_start);
        if (p_read)
            memcpy((uint8_t *) p_read + i_copy, (uint8_t*) t->p_data + i_start, i_count);
        i_copy += i_count;
    }

    p_sys->i_offset += i_copy;

    return i_copy;
}

static int Peek(stream_t *s, const uint8_t **pp_peek, unsigned int i_peek)
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
    /* ensure the data is ready to read */
    int64_t i_size = (p_sys->i_offset + i_peek > p_sys->i_size) ? p_sys->i_size : (p_sys->i_offset + i_peek);
    if (EnsureStreamParsed(s, i_size) < 0)
    {
        msg_Err(s, "Could not read to virtual offset %"PRId64" bytes", p_sys->i_offset);
        return 0;
    }
    /* the real work */
    unsigned int i_copy = 0;
    while (i_copy < i_peek)
    {
        int i_tag = GetFlvTagIndex(s, p_sys->i_offset + i_copy);
        flv_tag_t *t = vlc_array_item_at_index(p_sys->p_flv_tag, i_tag);
        if (ReadFlvTag(s, i_tag) < 0)
        {
            msg_Err(s, "Could not read data of tag #%d (offset = %"PRId64", size = %d)", i_tag, t->i_offset, t->i_size);
            break;
        }
        int i_stream = GetStreamIndex(s, p_sys->i_offset + i_copy);
        suburl_t *su = vlc_array_item_at_index(p_sys->p_video, i_stream);
        int i_start = (p_sys->i_offset > t->i_offset) ? (p_sys->i_offset - t->i_offset) : 0;
        int i_count = (i_peek - i_copy) < (t->i_size - i_start) ? (i_peek - i_copy) : (t->i_size - i_start);
        memcpy((uint8_t *) p_sys->p_peek + i_copy, (uint8_t*) t->p_data + i_start, i_count);
        i_copy += i_count;
    }
    *pp_peek = p_sys->p_peek;

    return i_copy;
}

static int Seek(stream_t *s, int64_t i_offset)
{
    stream_sys_t *p_sys = s->p_sys;

    if (i_offset >= p_sys->i_size)
        return VLC_EGENERIC;
    if (EnsureStreamParsed(s, i_offset) < 0)
        return VLC_EGENERIC;
    p_sys->i_offset = i_offset;

    return VLC_SUCCESS;
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
            return Seek(s, new_offset);
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

static void FlvMetaDataSetDuration(void *p_tag, int i_tag_size, int64_t i_value)
{
    const char *psz_name = "duration";
    int i_name_length = strlen(psz_name);
    /* string 10 onMetaData */
    static uint8_t onMetaData[] = {
        0x02, 0x00, 0x0a, 0x6f, 0x6e, 0x4d, 0x65, 0x74, 0x61, 0x44, 0x61, 0x74, 0x61
    };
    /* onMetaData + ECMA array + key + value*/
    if (i_tag_size < sizeof(onMetaData) + 5 + 2 + i_name_length + 1 + sizeof(double))
        return;
    if (memcmp(p_tag, onMetaData, sizeof(onMetaData)))
        return;
    uint8_t *p_array = ((uint8_t *) p_tag) + sizeof(onMetaData);
    if (*p_array != SCRIPT_DATA_ECMA)
        return;
    uint8_t *p_end = (uint8_t *) p_tag + i_tag_size;
    uint8_t *p_element = p_array + 1 + sizeof(uint32_t);
    while (p_element < p_end)
    {
        /* array key */
        int i_key_length = FLV_UI16(p_element);
        if ((i_key_length == i_name_length) && !memcmp(p_element + 2, psz_name, i_name_length) && (*(p_element + 2 + i_key_length) == SCRIPT_DATA_NUMBER))
        {
            uint8_t *p_old_value = p_element + 2 + i_key_length + 1;
            double d_value;
            memcpy(&d_value, p_old_value, sizeof(d_value));
            /* change edian */
            uint8_t *p_value = (uint8_t *) &d_value;
            for (int i = 0; i < sizeof(d_value); i++) {
                p_value[sizeof(d_value) - 1 - i] = p_old_value[i];
            }
            __android_log_print(ANDROID_LOG_DEBUG, "faplayer", "old duration = %.3f", d_value);
            /* change edian */
            d_value = (double) i_value / 1000.0;
            __android_log_print(ANDROID_LOG_DEBUG, "faplayer", "new duration = %.3f", d_value);
            p_value = (uint8_t *) &d_value;
            for (int i = 0; i < sizeof(d_value) / 2; i++) {
                uint8_t tmp = p_value[i];
                p_value[i] = p_value[sizeof(d_value) - 1 - i];
                p_value[sizeof(d_value) - 1 - i] = tmp;
            }
            /* write it */
            memcpy(p_old_value, &d_value, sizeof(d_value));
            break;
        }
        /* array value */
        p_element += (2 + i_key_length);
        int i_val_length = 1;
        switch (*p_element)
        {
        case SCRIPT_DATA_NUMBER:
            i_val_length += sizeof(double);
            break;
        case SCRIPT_DATA_BOOLEAN:
            i_val_length += sizeof(uint8_t);
            break;
        case SCRIPT_DATA_STRING:
            i_val_length += 2;
            i_val_length += FLV_UI16((p_element + 1));
            break;
        default:
            // XXX: not implemented yet
            return;
        }
        p_element += i_val_length;
    }
}

