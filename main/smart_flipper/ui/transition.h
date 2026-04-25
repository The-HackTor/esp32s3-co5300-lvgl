#ifndef TRANSITION_H
#define TRANSITION_H

#include <lvgl.h>
#include <stdint.h>

typedef enum {
    TransitionNone,
    TransitionSlideLeft,   /* Push new view from right (forward nav) */
    TransitionSlideRight,  /* Slide new view from left (back nav) */
    TransitionFadeIn,
} TransitionType;

/*
 * Animate switching from one view to another.
 * The outgoing view animates out, the incoming view animates in.
 * Duration in ms (recommend 150-200).
 * Pass NULL for outgoing if there's nothing to animate out.
 */
void transition_apply(lv_obj_t *outgoing, lv_obj_t *incoming,
                      TransitionType type, uint32_t duration_ms);

#endif
