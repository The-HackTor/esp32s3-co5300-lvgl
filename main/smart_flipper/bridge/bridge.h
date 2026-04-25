/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * UART DMA bridge transport to ESP32-C5.
 */

#ifndef BRIDGE_H
#define BRIDGE_H

#include <stdint.h>

int bridge_init(void);
int bridge_send(uint8_t type, const void *payload, uint16_t len);

#endif /* BRIDGE_H */
