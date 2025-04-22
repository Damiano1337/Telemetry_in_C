#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include "esp_err.h"

esp_err_t sd_logger_init(void);
void sd_logger_append(const char *json_line);
void sd_logger_flush(void);

#endif // SD_LOGGER_H