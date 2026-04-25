#include "view_custom.h"
#include "ui/styles.h"
#include <stdlib.h>

struct ViewCustom {
    lv_obj_t *root;
};

static lv_obj_t *custom_get_view(void *m) { return ((ViewCustom *)m)->root; }

static void custom_reset(void *m)
{
    ViewCustom *vc = m;
    lv_obj_clean(vc->root);
}

static void custom_destroy(void *m)
{
    ViewCustom *vc = m;
    if(vc->root) { lv_obj_delete(vc->root); vc->root = NULL; }
    free(vc);
}

static const ViewModuleVtable custom_vtable = {
    .get_view = custom_get_view,
    .reset    = custom_reset,
    .destroy  = custom_destroy,
};

ViewCustom *view_custom_alloc(lv_obj_t *parent)
{
    ViewCustom *vc = calloc(1, sizeof(ViewCustom));

    vc->root = lv_obj_create(parent);
    lv_obj_add_flag(vc->root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(vc->root, DISP_W, DISP_H);
    lv_obj_set_style_bg_color(vc->root, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(vc->root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(vc->root, 0, 0);
    lv_obj_set_style_pad_all(vc->root, 0, 0);
    lv_obj_remove_flag(vc->root, LV_OBJ_FLAG_SCROLLABLE);

    return vc;
}

void view_custom_free(ViewCustom *vc) { custom_destroy(vc); }

ViewModule view_custom_get_module(ViewCustom *vc)
{
    return (ViewModule){ .module = vc, .vtable = &custom_vtable };
}

lv_obj_t *view_custom_get_view(ViewCustom *vc) { return vc->root; }

void view_custom_clean(ViewCustom *vc) { custom_reset(vc); }
