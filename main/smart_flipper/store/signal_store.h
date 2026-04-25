#ifndef SIGNAL_STORE_H
#define SIGNAL_STORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SIGNAL_STORE_MAX_SLOTS        32
#define SIGNAL_STORE_MAX_SAVE_SAMPLES 2000
#define SIGNAL_NAME_MAX               32
#define SIGNAL_STORE_SAVED_DIR        "/lfs/subghz/saved"
#define SIGNAL_STORE_SLOT_PATH_LEN    80

struct signal_meta {
	uint32_t freq_khz;
	uint8_t  preset;
	uint16_t sample_count;   /* preview sample count in file */
	uint32_t full_count;     /* full sample count streamed to file */
	char     protocol[16];
	uint64_t decoded_data;
	uint8_t  decoded_bits;
};

void signal_store_slot_path(uint8_t slot, char *out, size_t len);
int  signal_store_save_file(uint8_t slot, const char *name,
			    const struct signal_meta *meta,
			    const char *src_file_path);

int  signal_store_init(void);
int  signal_store_save(uint8_t slot, const char *name,
		       const struct signal_meta *meta,
		       const int32_t *samples, uint16_t count);
int  signal_store_load(uint8_t slot, char *name, size_t name_len,
		       struct signal_meta *meta,
		       int32_t *samples, uint16_t max_count);
int  signal_store_delete(uint8_t slot);
int  signal_store_list(void (*print_fn)(uint8_t slot, const char *name));
int  signal_store_count(void);
bool signal_store_has(uint8_t slot);
int  signal_store_find_free_slot(void);
void signal_store_auto_name(char *name, size_t len,
			     const struct signal_meta *meta);

#endif
