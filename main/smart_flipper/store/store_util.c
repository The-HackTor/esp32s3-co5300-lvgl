#include "store_util.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define STORE_TMP_SUFFIX     ".tmp"
#define STORE_PATH_MAX_BYTES 256

static bool build_tmp_path(const char *final_path, char *out, size_t out_sz)
{
    if(!final_path || !out) return false;
    size_t n = strlen(final_path);
    if(n + sizeof(STORE_TMP_SUFFIX) > out_sz) return false;
    memcpy(out, final_path, n);
    memcpy(out + n, STORE_TMP_SUFFIX, sizeof(STORE_TMP_SUFFIX));
    return true;
}

FILE *store_atomic_open(const char *final_path)
{
    char tmp[STORE_PATH_MAX_BYTES];
    if(!build_tmp_path(final_path, tmp, sizeof(tmp))) return NULL;
    return fopen(tmp, "w");
}

esp_err_t store_atomic_commit(FILE *fp, const char *final_path)
{
    char tmp[STORE_PATH_MAX_BYTES];
    if(!fp || !build_tmp_path(final_path, tmp, sizeof(tmp))) {
        if(fp) fclose(fp);
        return ESP_ERR_INVALID_ARG;
    }
    if(fflush(fp) != 0) {
        fclose(fp);
        unlink(tmp);
        return ESP_FAIL;
    }
    if(fclose(fp) != 0) {
        unlink(tmp);
        return ESP_FAIL;
    }
    if(rename(tmp, final_path) != 0) {
        unlink(tmp);
        return ESP_FAIL;
    }
    return ESP_OK;
}

void store_atomic_abort(FILE *fp, const char *final_path)
{
    if(fp) fclose(fp);
    char tmp[STORE_PATH_MAX_BYTES];
    if(build_tmp_path(final_path, tmp, sizeof(tmp))) {
        unlink(tmp);
    }
}
