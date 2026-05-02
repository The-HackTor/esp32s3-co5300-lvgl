#include "macro_store.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_log.h"
#include "store_util.h"

#define TAG "macro_store"

#define LINE_BUF_BYTES   512
#define STEPS_INIT_CAP   4

static const char IR_MACRO_FILETYPE[] = "Filetype: IR macro file";
static const char IR_MACRO_VERSION[]  = "Version: 1";

static bool starts_with(const char *line, const char *prefix)
{
    return strncmp(line, prefix, strlen(prefix)) == 0;
}

static const char *skip_ws(const char *p)
{
    while(*p == ' ' || *p == '\t') p++;
    return p;
}

static void rstrip(char *s)
{
    size_t n = strlen(s);
    while(n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r' ||
                    s[n - 1] == ' '  || s[n - 1] == '\t')) {
        s[--n] = '\0';
    }
}

static esp_err_t ensure_dir(const char *path)
{
    struct stat st;
    if(stat(path, &st) == 0) return S_ISDIR(st.st_mode) ? ESP_OK : ESP_FAIL;
    if(mkdir(path, 0775) != 0) {
        ESP_LOGW(TAG, "mkdir %s failed", path);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t macro_store_init(void)
{
    return ensure_dir(IR_MACRO_DIR);
}

esp_err_t macro_store_path(char *out, size_t out_len, const char *name)
{
    if(!out || !name || out_len == 0) return ESP_ERR_INVALID_ARG;
    int n = snprintf(out, out_len, "%s/%s.macro", IR_MACRO_DIR, name);
    if(n < 0 || (size_t)n >= out_len) return ESP_ERR_INVALID_SIZE;
    return ESP_OK;
}

esp_err_t macro_store_list(char (*out_names)[IR_MACRO_NAME_MAX],
                           size_t cap, size_t *out_count)
{
    if(!out_names || !out_count) return ESP_ERR_INVALID_ARG;
    *out_count = 0;

    DIR *dir = opendir(IR_MACRO_DIR);
    if(!dir) return ESP_ERR_NOT_FOUND;

    struct dirent *ent;
    while((ent = readdir(dir)) != NULL) {
        if(*out_count >= cap) break;
        const char *dot = strrchr(ent->d_name, '.');
        if(!dot || strcmp(dot, ".macro") != 0) continue;

        size_t base_len = (size_t)(dot - ent->d_name);
        if(base_len >= IR_MACRO_NAME_MAX) base_len = IR_MACRO_NAME_MAX - 1;
        memcpy(out_names[*out_count], ent->d_name, base_len);
        out_names[*out_count][base_len] = '\0';
        (*out_count)++;
    }
    closedir(dir);
    return ESP_OK;
}

esp_err_t macro_init(IrMacro *m, const char *name)
{
    if(!m || !name) return ESP_ERR_INVALID_ARG;
    memset(m, 0, sizeof(*m));
    snprintf(m->name, sizeof(m->name), "%s", name);
    return macro_store_path(m->path, sizeof(m->path), name);
}

void macro_free(IrMacro *m)
{
    if(!m) return;
    if(m->steps) free(m->steps);
    m->steps = NULL;
    m->step_count = 0;
    m->step_cap = 0;
}

static esp_err_t steps_grow(IrMacro *m)
{
    if(m->step_count >= IR_MACRO_STEP_MAX) return ESP_ERR_NO_MEM;
    size_t new_cap = (m->step_cap == 0) ? STEPS_INIT_CAP : m->step_cap * 2;
    if(new_cap > IR_MACRO_STEP_MAX) new_cap = IR_MACRO_STEP_MAX;
    IrMacroStep *s = realloc(m->steps, new_cap * sizeof(IrMacroStep));
    if(!s) return ESP_ERR_NO_MEM;
    m->steps   = s;
    m->step_cap = new_cap;
    return ESP_OK;
}

esp_err_t macro_append_step(IrMacro *m, const IrMacroStep *s)
{
    if(!m || !s) return ESP_ERR_INVALID_ARG;
    if(m->step_count == m->step_cap) {
        esp_err_t err = steps_grow(m);
        if(err != ESP_OK) return err;
    }
    m->steps[m->step_count++] = *s;
    return ESP_OK;
}

esp_err_t macro_delete_step(IrMacro *m, size_t idx)
{
    if(!m || idx >= m->step_count) return ESP_ERR_INVALID_ARG;
    if(idx < m->step_count - 1) {
        memmove(&m->steps[idx], &m->steps[idx + 1],
                (m->step_count - idx - 1) * sizeof(IrMacroStep));
    }
    m->step_count--;
    return ESP_OK;
}

esp_err_t macro_move_step(IrMacro *m, size_t from, size_t to)
{
    if(!m || from >= m->step_count || to >= m->step_count) return ESP_ERR_INVALID_ARG;
    if(from == to) return ESP_OK;
    IrMacroStep tmp = m->steps[from];
    if(from < to) {
        memmove(&m->steps[from], &m->steps[from + 1],
                (to - from) * sizeof(IrMacroStep));
    } else {
        memmove(&m->steps[to + 1], &m->steps[to],
                (from - to) * sizeof(IrMacroStep));
    }
    m->steps[to] = tmp;
    return ESP_OK;
}

esp_err_t macro_load(IrMacro *out, const char *path)
{
    if(!out || !path) return ESP_ERR_INVALID_ARG;

    FILE *fp = fopen(path, "r");
    if(!fp) return ESP_ERR_NOT_FOUND;

    memset(out, 0, sizeof(*out));
    snprintf(out->path, sizeof(out->path), "%s", path);

    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    snprintf(out->name, sizeof(out->name), "%s", base);
    char *dot = strrchr(out->name, '.');
    if(dot) *dot = '\0';

    char line[LINE_BUF_BYTES];
    IrMacroStep cur = {0};
    bool cur_valid = false;
    bool header_done = false;

    while(fgets(line, sizeof(line), fp)) {
        rstrip(line);
        const char *p = skip_ws(line);

        if(*p == '\0') continue;
        if(!header_done) {
            if(starts_with(p, IR_MACRO_FILETYPE) ||
               starts_with(p, IR_MACRO_VERSION)) continue;
            if(*p == '#') { header_done = true; cur_valid = true; }
            continue;
        }
        if(*p == '#') {
            if(cur_valid && cur.remote[0] && cur.button[0]) {
                if(macro_append_step(out, &cur) != ESP_OK) break;
            }
            memset(&cur, 0, sizeof(cur));
            cur_valid = true;
            continue;
        }

        if(starts_with(p, "remote:")) {
            snprintf(cur.remote, sizeof(cur.remote), "%s", skip_ws(p + 7));
        } else if(starts_with(p, "button:")) {
            snprintf(cur.button, sizeof(cur.button), "%s", skip_ws(p + 7));
        } else if(starts_with(p, "delay_ms:")) {
            cur.delay_ms = (uint32_t)atoi(skip_ws(p + 9));
        } else if(starts_with(p, "repeat:")) {
            int rp = atoi(skip_ws(p + 7));
            if(rp < 1) rp = 1;
            if(rp > 99) rp = 99;
            cur.repeat = (uint8_t)rp;
        }
    }
    if(cur_valid && cur.remote[0] && cur.button[0]) {
        macro_append_step(out, &cur);
    }

    fclose(fp);
    return ESP_OK;
}

esp_err_t macro_save(const IrMacro *in)
{
    if(!in || in->path[0] == '\0') return ESP_ERR_INVALID_ARG;

    FILE *fp = store_atomic_open(in->path);
    if(!fp) return ESP_FAIL;

    if(fprintf(fp, "%s\n%s\n", IR_MACRO_FILETYPE, IR_MACRO_VERSION) < 0) {
        store_atomic_abort(fp, in->path);
        return ESP_FAIL;
    }
    for(size_t i = 0; i < in->step_count; i++) {
        const IrMacroStep *s = &in->steps[i];
        uint8_t rp = s->repeat ? s->repeat : 1;
        if(fprintf(fp, "#\nremote: %s\nbutton: %s\ndelay_ms: %lu\nrepeat: %u\n",
                   s->remote, s->button, (unsigned long)s->delay_ms, rp) < 0) {
            store_atomic_abort(fp, in->path);
            return ESP_FAIL;
        }
    }
    return store_atomic_commit(fp, in->path);
}

esp_err_t macro_delete_file(const IrMacro *m)
{
    if(!m || m->path[0] == '\0') return ESP_ERR_INVALID_ARG;
    if(unlink(m->path) != 0) return ESP_FAIL;
    return ESP_OK;
}
