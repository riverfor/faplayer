
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jni.h>
#include <android/log.h>

static char *psz_self_path = NULL;

static char *psz_libraries[] = 
{
    "libiconv.so",
    "libfreetype.so",
    "libavutil.so",
    "libavcodec.so",
    "libavformat.so",
    "libswscale.so"
};

static ssize_t getdelim (char **restrict lineptr, size_t *restrict n,
                  int delimiter, FILE *restrict stream)
{
    char *ptr = *lineptr;
    size_t size = (ptr != NULL) ? *n : 0;
    size_t len = 0;

    for (;;)
    {
        if ((size - len) <= 2)
        {
            size = size ? (size * 2) : 256;
            ptr = realloc (*lineptr, size);
            if (ptr == NULL)
                return -1;
            *lineptr = ptr;
            *n = size;
        }

        int c = fgetc (stream);
        if (c == -1)
        {
            if (len == 0 || ferror (stream))
                return -1;
            break; /* EOF */
        }
        ptr[len++] = c;
        if (c == delimiter)
            break;
    }

    ptr[len] = '\0';
    return len;
}

static ssize_t getline (char **restrict lineptr, size_t *restrict n,
                 FILE *restrict stream)
{
    return getdelim (lineptr, n, '\n', stream);
}

static void set_self_path (void)
{
    /* Find the path to libvlc (i.e. ourselves) */
    FILE *maps = fopen ("/proc/self/maps", "rt");
    if (maps == NULL)
        goto error;

    char *line = NULL;
    size_t linelen = 0;
    uintptr_t needle = (uintptr_t)set_self_path;

    for (;;)
    {
        ssize_t len = getline (&line, &linelen, maps);
        if (len == -1)
            break;

        void *start, *end;
        if (sscanf (line, "%p-%p", &start, &end) < 2)
            continue;
        if (needle < (uintptr_t)start || (uintptr_t)end <= needle)
            continue;
        char *dir = strchr (line, '/');
        if (dir == NULL)
            continue;
        char *file = strrchr (line, '/');
        if (end == NULL)
            continue;
        *file = '\0';
        if (asprintf (&psz_self_path, "%s/", dir) == -1)
            goto error;
        break;
    }
    free (line);
    fclose (maps);
    return;

error:
    psz_self_path = NULL;
}

static void clr_self_path()
{
    if (psz_self_path)
    {
        free (psz_self_path);
        psz_self_path = NULL;
    }
}

static void load_libraries()
{
    if (psz_self_path == NULL)
        return;
    for (int i = 0; i < sizeof(psz_libraries)/sizeof(psz_libraries[0]); i++)
    {
        char *psz_library_path;
        int err = asprintf(&psz_library_path, "%s/%s", psz_self_path, psz_libraries[i]);
        if (err < 0)
        {
            __android_log_print(ANDROID_LOG_ERROR, "faplayer", "failed to allocate memory for `%s'.", psz_libraries[i]);
            continue;
        }
        void *p_handle = dlopen(psz_library_path, RTLD_NOW);
        free(psz_library_path);
        if (!p_handle)
        {
            __android_log_print(ANDROID_LOG_ERROR, "faplayer", "failed to dlopen `%s'.", psz_library_path);
            continue;
        }
    }
}

static void set_vlc_plugin_path()
{
    if (psz_self_path == NULL)
        return;
    setenv("VLC_PLUGIN_PATH", psz_self_path, 1);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    set_self_path();
    load_libraries();
    /* XXX: not working */
    set_vlc_plugin_path();
    clr_self_path();

    return JNI_VERSION_1_2;
}

