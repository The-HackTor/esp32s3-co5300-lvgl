#include "codec_db_send.h"

#include <string.h>

typedef struct {
    const char       *name;
    ir_codec_send_fn  fn;
} send_entry_t;

static const send_entry_t s_table[] = {
    { "JVC",          ir_jvc_send   },
    { "Sharp",        ir_sharp_send },
    { "Denon",        ir_denon_send },
    { "Aiwa RC-T501", ir_aiwa_send  },
    { "Nikai",        ir_nikai_send },
};

ir_codec_send_fn codec_db_send_lookup(const char *protocol_name)
{
    if(!protocol_name) return NULL;
    const size_t n = sizeof(s_table) / sizeof(s_table[0]);
    for(size_t i = 0; i < n; i++) {
        if(strcmp(s_table[i].name, protocol_name) == 0) return s_table[i].fn;
    }
    return NULL;
}
