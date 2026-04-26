#include "ir_store.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_log.h"

#define TAG "ir_store"

#define LINE_BUF_BYTES   (16 * 1024)
#define BUTTONS_INIT_CAP 4
#define TIMINGS_INIT_CAP 64

static const char IR_FILETYPE_SIGNALS[] = "Filetype: IR signals file";
static const char IR_FILETYPE_LIBRARY[] = "Filetype: IR library file";
static const char IR_VERSION_HEADER[]   = "Version: 1";

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

static esp_err_t parse_hex32_le(const char *s, uint32_t *out)
{
    unsigned b0, b1, b2, b3;
    if(sscanf(s, "%x %x %x %x", &b0, &b1, &b2, &b3) != 4) return ESP_ERR_INVALID_ARG;
    *out = (uint32_t)b0 | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
    return ESP_OK;
}

static esp_err_t parse_timings(const char *s, uint16_t **out_arr, size_t *out_n)
{
    size_t cap = TIMINGS_INIT_CAP;
    size_t n   = 0;
    uint16_t *arr = malloc(cap * sizeof(uint16_t));
    if(!arr) return ESP_ERR_NO_MEM;

    const char *p = s;
    while(*p) {
        p = skip_ws(p);
        if(!isdigit((unsigned char)*p)) break;

        unsigned v;
        int consumed;
        if(sscanf(p, "%u%n", &v, &consumed) != 1) break;
        if(v > 0xFFFF) v = 0xFFFF;

        if(n == cap) {
            size_t new_cap = cap * 2;
            uint16_t *r = realloc(arr, new_cap * sizeof(uint16_t));
            if(!r) { free(arr); return ESP_ERR_NO_MEM; }
            arr = r;
            cap = new_cap;
        }
        arr[n++] = (uint16_t)v;
        p += consumed;
    }

    if(n == 0) { free(arr); return ESP_ERR_INVALID_ARG; }
    *out_arr = arr;
    *out_n   = n;
    return ESP_OK;
}

static esp_err_t buttons_grow(IrRemote *r)
{
    size_t new_cap = (r->button_cap == 0) ? BUTTONS_INIT_CAP : r->button_cap * 2;
    IrButton *b = realloc(r->buttons, new_cap * sizeof(IrButton));
    if(!b) return ESP_ERR_NO_MEM;
    r->buttons   = b;
    r->button_cap = new_cap;
    return ESP_OK;
}

void ir_button_free(IrButton *btn)
{
    if(!btn) return;
    if(btn->signal.type == INFRARED_SIGNAL_RAW && btn->signal.raw.timings) {
        free(btn->signal.raw.timings);
        btn->signal.raw.timings = NULL;
        btn->signal.raw.n_timings = 0;
    }
}

esp_err_t ir_button_dup(IrButton *dst, const IrButton *src)
{
    if(!dst || !src) return ESP_ERR_INVALID_ARG;
    *dst = *src;
    if(src->signal.type == INFRARED_SIGNAL_RAW && src->signal.raw.timings) {
        size_t bytes = src->signal.raw.n_timings * sizeof(uint16_t);
        dst->signal.raw.timings = malloc(bytes);
        if(!dst->signal.raw.timings) return ESP_ERR_NO_MEM;
        memcpy(dst->signal.raw.timings, src->signal.raw.timings, bytes);
    }
    return ESP_OK;
}

void ir_remote_free(IrRemote *r)
{
    if(!r || !r->buttons) return;
    for(size_t i = 0; i < r->button_count; i++) ir_button_free(&r->buttons[i]);
    free(r->buttons);
    r->buttons = NULL;
    r->button_count = 0;
    r->button_cap = 0;
    r->dirty = false;
}

esp_err_t ir_remote_init(IrRemote *r, const char *name)
{
    if(!r || !name) return ESP_ERR_INVALID_ARG;
    memset(r, 0, sizeof(*r));
    strncpy(r->name, name, sizeof(r->name) - 1);
    return ir_store_remote_path(r->path, sizeof(r->path), name);
}

esp_err_t ir_remote_append_button(IrRemote *r, const IrButton *btn)
{
    if(!r || !btn) return ESP_ERR_INVALID_ARG;
    if(r->button_count == r->button_cap) {
        esp_err_t err = buttons_grow(r);
        if(err != ESP_OK) return err;
    }
    esp_err_t err = ir_button_dup(&r->buttons[r->button_count], btn);
    if(err != ESP_OK) return err;
    r->button_count++;
    r->dirty = true;
    return ESP_OK;
}

esp_err_t ir_remote_delete_button(IrRemote *r, size_t idx)
{
    if(!r || idx >= r->button_count) return ESP_ERR_INVALID_ARG;
    ir_button_free(&r->buttons[idx]);
    if(idx < r->button_count - 1) {
        memmove(&r->buttons[idx], &r->buttons[idx + 1],
                (r->button_count - idx - 1) * sizeof(IrButton));
    }
    r->button_count--;
    r->dirty = true;
    return ESP_OK;
}

esp_err_t ir_remote_rename_button(IrRemote *r, size_t idx, const char *new_name)
{
    if(!r || !new_name || idx >= r->button_count) return ESP_ERR_INVALID_ARG;
    snprintf(r->buttons[idx].name, sizeof(r->buttons[idx].name), "%s", new_name);
    r->dirty = true;
    return ESP_OK;
}

esp_err_t ir_remote_move_button(IrRemote *r, size_t from, size_t to)
{
    if(!r || from >= r->button_count || to >= r->button_count) return ESP_ERR_INVALID_ARG;
    if(from == to) return ESP_OK;
    IrButton tmp = r->buttons[from];
    if(from < to) {
        memmove(&r->buttons[from], &r->buttons[from + 1],
                (to - from) * sizeof(IrButton));
    } else {
        memmove(&r->buttons[to + 1], &r->buttons[to],
                (from - to) * sizeof(IrButton));
    }
    r->buttons[to] = tmp;
    r->dirty = true;
    return ESP_OK;
}

esp_err_t ir_remote_rename(IrRemote *r, const char *new_name)
{
    if(!r || !new_name || new_name[0] == '\0') return ESP_ERR_INVALID_ARG;
    if(strcmp(r->name, new_name) == 0) return ESP_OK;

    char new_path[IR_REMOTE_PATH_MAX];
    esp_err_t err = ir_store_remote_path(new_path, sizeof(new_path), new_name);
    if(err != ESP_OK) return err;

    char old_path[IR_REMOTE_PATH_MAX];
    snprintf(old_path, sizeof(old_path), "%s", r->path);

    snprintf(r->name, sizeof(r->name), "%s", new_name);
    snprintf(r->path, sizeof(r->path), "%s", new_path);

    err = ir_remote_save(r);
    if(err != ESP_OK) return err;

    if(old_path[0] && strcmp(old_path, new_path) != 0) {
        unlink(old_path);
    }
    r->dirty = false;
    return ESP_OK;
}

esp_err_t ir_remote_delete_file(const IrRemote *r)
{
    if(!r || r->path[0] == '\0') return ESP_ERR_INVALID_ARG;
    if(unlink(r->path) != 0) return ESP_FAIL;
    return ESP_OK;
}

static void finalize_button(IrRemote *r, IrButton *cur, bool *cur_valid)
{
    if(!*cur_valid) return;
    if(r->button_count == r->button_cap) {
        if(buttons_grow(r) != ESP_OK) {
            ir_button_free(cur);
            *cur_valid = false;
            return;
        }
    }
    r->buttons[r->button_count++] = *cur;
    memset(cur, 0, sizeof(*cur));
    *cur_valid = false;
}

static esp_err_t parse_remote_stream(IrRemote *out, FILE *fp)
{
    char *line = malloc(LINE_BUF_BYTES);
    if(!line) return ESP_ERR_NO_MEM;

    IrButton cur = {0};
    bool cur_valid = false;
    bool header_done = false;

    while(fgets(line, LINE_BUF_BYTES, fp)) {
        rstrip(line);
        const char *p = skip_ws(line);

        if(*p == '\0' || *p == '#' || !header_done) {
            if(!header_done) {
                if(starts_with(p, IR_FILETYPE_SIGNALS) ||
                   starts_with(p, IR_FILETYPE_LIBRARY) ||
                   starts_with(p, IR_VERSION_HEADER)) continue;
                if(*p == '#') header_done = true;
                continue;
            }
            if(*p == '#') {
                finalize_button(out, &cur, &cur_valid);
                cur_valid = true;
                continue;
            }
            continue;
        }

        if(starts_with(p, "name:")) {
            const char *v = skip_ws(p + 5);
            strncpy(cur.name, v, sizeof(cur.name) - 1);
            cur.name[sizeof(cur.name) - 1] = '\0';
        } else if(starts_with(p, "type:")) {
            const char *v = skip_ws(p + 5);
            cur.signal.type = (strcmp(v, "raw") == 0)
                              ? INFRARED_SIGNAL_RAW
                              : INFRARED_SIGNAL_PARSED;
        } else if(starts_with(p, "protocol:")) {
            const char *v = skip_ws(p + 9);
            strncpy(cur.signal.parsed.protocol, v,
                    sizeof(cur.signal.parsed.protocol) - 1);
            cur.signal.parsed.protocol[sizeof(cur.signal.parsed.protocol) - 1] = '\0';
        } else if(starts_with(p, "address:")) {
            parse_hex32_le(skip_ws(p + 8), &cur.signal.parsed.address);
        } else if(starts_with(p, "command:")) {
            parse_hex32_le(skip_ws(p + 8), &cur.signal.parsed.command);
        } else if(starts_with(p, "frequency:")) {
            cur.signal.raw.freq_hz = (uint32_t)atoi(skip_ws(p + 10));
        } else if(starts_with(p, "duty_cycle:")) {
            float duty = strtof(skip_ws(p + 11), NULL);
            cur.signal.raw.duty_pct = (uint16_t)(duty * 100.0f + 0.5f);
        } else if(starts_with(p, "data:")) {
            uint16_t *arr = NULL;
            size_t n = 0;
            if(parse_timings(skip_ws(p + 5), &arr, &n) == ESP_OK) {
                if(cur.signal.raw.timings) free(cur.signal.raw.timings);
                cur.signal.raw.timings   = arr;
                cur.signal.raw.n_timings = n;
            }
        }
    }
    finalize_button(out, &cur, &cur_valid);

    free(line);
    out->dirty = false;
    return ESP_OK;
}

esp_err_t ir_remote_load(IrRemote *out, const char *path)
{
    if(!out || !path) return ESP_ERR_INVALID_ARG;

    FILE *fp = fopen(path, "r");
    if(!fp) return ESP_ERR_NOT_FOUND;

    memset(out, 0, sizeof(*out));
    strncpy(out->path, path, sizeof(out->path) - 1);

    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    strncpy(out->name, base, sizeof(out->name) - 1);
    char *dot = strrchr(out->name, '.');
    if(dot) *dot = '\0';

    esp_err_t err = parse_remote_stream(out, fp);
    fclose(fp);
    return err;
}

esp_err_t ir_remote_load_blob(IrRemote *out, const char *name,
                              const char *buf, size_t buf_len)
{
    if(!out || !buf || buf_len == 0) return ESP_ERR_INVALID_ARG;

    FILE *fp = fmemopen((void *)buf, buf_len, "r");
    if(!fp) return ESP_FAIL;

    memset(out, 0, sizeof(*out));
    if(name) {
        strncpy(out->name, name, sizeof(out->name) - 1);
    }

    esp_err_t err = parse_remote_stream(out, fp);
    fclose(fp);
    return err;
}

static void write_hex32_le(FILE *fp, const char *key, uint32_t v)
{
    fprintf(fp, "%s: %02X %02X %02X %02X\n",
            key,
            (unsigned)(v & 0xFF),
            (unsigned)((v >> 8) & 0xFF),
            (unsigned)((v >> 16) & 0xFF),
            (unsigned)((v >> 24) & 0xFF));
}

esp_err_t ir_remote_save(const IrRemote *in)
{
    if(!in || in->path[0] == '\0') return ESP_ERR_INVALID_ARG;

    FILE *fp = fopen(in->path, "w");
    if(!fp) return ESP_FAIL;

    fprintf(fp, "%s\n%s\n", IR_FILETYPE_SIGNALS, IR_VERSION_HEADER);

    for(size_t i = 0; i < in->button_count; i++) {
        const IrButton *b = &in->buttons[i];
        fprintf(fp, "#\nname: %s\n", b->name);
        if(b->signal.type == INFRARED_SIGNAL_PARSED) {
            fprintf(fp, "type: parsed\n");
            fprintf(fp, "protocol: %s\n", b->signal.parsed.protocol);
            write_hex32_le(fp, "address", b->signal.parsed.address);
            write_hex32_le(fp, "command", b->signal.parsed.command);
        } else {
            fprintf(fp, "type: raw\n");
            fprintf(fp, "frequency: %lu\n", (unsigned long)b->signal.raw.freq_hz);
            fprintf(fp, "duty_cycle: %.6f\n", b->signal.raw.duty_pct / 100.0f);
            fprintf(fp, "data:");
            for(size_t j = 0; j < b->signal.raw.n_timings; j++) {
                fprintf(fp, " %u", b->signal.raw.timings[j]);
            }
            fprintf(fp, "\n");
        }
    }

    fclose(fp);
    return ESP_OK;
}

static esp_err_t ensure_dir(const char *path)
{
    struct stat st;
    if(stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode) ? ESP_OK : ESP_FAIL;
    }
    if(mkdir(path, 0775) != 0) {
        ESP_LOGW(TAG, "mkdir %s failed", path);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t ir_store_init(void)
{
    if(ensure_dir(IR_STORE_DIR) != ESP_OK) return ESP_FAIL;
    if(ensure_dir(IR_UNIVERSAL_DIR) != ESP_OK) return ESP_FAIL;
    char buf[IR_REMOTE_PATH_MAX];
    for(int c = 0; c < IR_UNIVERSAL_CAT_COUNT; c++) {
        snprintf(buf, sizeof(buf), "%s/%s",
                 IR_UNIVERSAL_DIR,
                 ir_universal_category_dirname((IrUniversalCategory)c));
        ensure_dir(buf);
    }
    return ESP_OK;
}

const char *ir_universal_category_dirname(IrUniversalCategory cat)
{
    switch(cat) {
    case IR_UNIVERSAL_CAT_TV:        return "tv";
    case IR_UNIVERSAL_CAT_AC:        return "ac";
    case IR_UNIVERSAL_CAT_AUDIO:     return "audio";
    case IR_UNIVERSAL_CAT_PROJECTOR: return "projector";
    default: return "";
    }
}

const char *ir_universal_category_label(IrUniversalCategory cat)
{
    switch(cat) {
    case IR_UNIVERSAL_CAT_TV:        return "TVs";
    case IR_UNIVERSAL_CAT_AC:        return "Air Conditioners";
    case IR_UNIVERSAL_CAT_AUDIO:     return "Audio Receivers";
    case IR_UNIVERSAL_CAT_PROJECTOR: return "Projectors";
    default: return "";
    }
}

esp_err_t ir_universal_path(IrUniversalCategory cat, const char *name,
                            char *out, size_t out_len)
{
    if(!out || !name || out_len == 0) return ESP_ERR_INVALID_ARG;
    int n = snprintf(out, out_len, "%s/%s/%s.ir",
                     IR_UNIVERSAL_DIR,
                     ir_universal_category_dirname(cat), name);
    if(n < 0 || (size_t)n >= out_len) return ESP_ERR_INVALID_SIZE;
    return ESP_OK;
}

esp_err_t ir_universal_list(IrUniversalCategory cat,
                            char (*out_names)[IR_REMOTE_NAME_MAX],
                            size_t cap, size_t *out_count)
{
    if(!out_names || !out_count) return ESP_ERR_INVALID_ARG;
    *out_count = 0;

    char path[IR_REMOTE_PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s",
             IR_UNIVERSAL_DIR, ir_universal_category_dirname(cat));

    DIR *dir = opendir(path);
    if(!dir) return ESP_ERR_NOT_FOUND;

    struct dirent *ent;
    while((ent = readdir(dir)) != NULL) {
        if(*out_count >= cap) break;
        const char *dot = strrchr(ent->d_name, '.');
        if(!dot || strcmp(dot, ".ir") != 0) continue;

        size_t base_len = (size_t)(dot - ent->d_name);
        if(base_len >= IR_REMOTE_NAME_MAX) base_len = IR_REMOTE_NAME_MAX - 1;
        memcpy(out_names[*out_count], ent->d_name, base_len);
        out_names[*out_count][base_len] = '\0';
        (*out_count)++;
    }
    closedir(dir);
    return ESP_OK;
}

esp_err_t ir_store_remote_path(char *out, size_t out_len, const char *name)
{
    if(!out || !name || out_len == 0) return ESP_ERR_INVALID_ARG;
    int n = snprintf(out, out_len, "%s/%s.ir", IR_STORE_DIR, name);
    if(n < 0 || (size_t)n >= out_len) return ESP_ERR_INVALID_SIZE;
    return ESP_OK;
}

esp_err_t ir_store_list_remotes(char (*out_names)[IR_REMOTE_NAME_MAX],
                                size_t cap, size_t *out_count)
{
    if(!out_names || !out_count) return ESP_ERR_INVALID_ARG;
    *out_count = 0;

    DIR *dir = opendir(IR_STORE_DIR);
    if(!dir) return ESP_ERR_NOT_FOUND;

    struct dirent *ent;
    while((ent = readdir(dir)) != NULL) {
        if(*out_count >= cap) break;
        const char *dot = strrchr(ent->d_name, '.');
        if(!dot || strcmp(dot, ".ir") != 0) continue;

        size_t base_len = (size_t)(dot - ent->d_name);
        if(base_len >= IR_REMOTE_NAME_MAX) base_len = IR_REMOTE_NAME_MAX - 1;
        memcpy(out_names[*out_count], ent->d_name, base_len);
        out_names[*out_count][base_len] = '\0';
        (*out_count)++;
    }
    closedir(dir);
    return ESP_OK;
}
