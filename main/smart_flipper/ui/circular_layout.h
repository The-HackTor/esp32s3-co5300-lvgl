#ifndef CIRCULAR_LAYOUT_H
#define CIRCULAR_LAYOUT_H

#include <stdint.h>

#define CIRCULAR_DISPLAY_RADIUS  233   /* 466 / 2 */
#define CIRCULAR_SAFE_PAD         40   /* Minimum padding from edge */
#define CIRCULAR_CENTER_WIDTH    420   /* Max usable width at center */

/*
 * Compute the safe horizontal width at a given vertical offset from
 * the display center. Uses the chord formula: w = 2 * sqrt(R^2 - y^2)
 * minus safe padding.
 *
 * Returns clamped to [min_width, CIRCULAR_CENTER_WIDTH].
 */
int32_t circular_safe_width(int32_t y_from_center, int32_t min_width);

/*
 * Compute a barrel-curve factor (0.0 .. 1.0 mapped to 0 .. 256)
 * for a given y offset from center. Can be used for opacity, scale, etc.
 *   256 = fully at center, 0 = at edge
 */
int32_t circular_curve_factor(int32_t y_from_center);

#endif
