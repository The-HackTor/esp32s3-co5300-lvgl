#ifndef STORE_UTIL_H
#define STORE_UTIL_H

#include <stdio.h>
#include "esp_err.h"

FILE     *store_atomic_open  (const char *final_path);
esp_err_t store_atomic_commit(FILE *fp, const char *final_path);
void      store_atomic_abort (FILE *fp, const char *final_path);

#endif
