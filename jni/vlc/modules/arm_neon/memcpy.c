
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_cpu.h>

extern void *memcpy7(void*, const void*, size_t);

static int Activate(vlc_object_t *p_this) {
    VLC_UNUSED(p_this);
    vlc_fastmem_register(memcpy7, NULL);

    return VLC_SUCCESS;
}

vlc_module_begin ()
    set_category(CAT_ADVANCED)
    set_subcategory(SUBCAT_ADVANCED_MISC)
    set_capability("memcpy", 50)
    set_callbacks(Activate, NULL)
vlc_module_end ()

