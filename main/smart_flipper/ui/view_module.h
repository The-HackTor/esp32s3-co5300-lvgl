#ifndef VIEW_MODULE_H
#define VIEW_MODULE_H

#include <lvgl.h>

typedef struct {
    lv_obj_t *(*get_view)(void *module);
    void      (*reset)(void *module);
    void      (*destroy)(void *module);
} ViewModuleVtable;

typedef struct {
    void                   *module;
    const ViewModuleVtable *vtable;
} ViewModule;

static inline lv_obj_t *view_module_get_view(ViewModule *vm)
{
    return vm->vtable->get_view(vm->module);
}

static inline void view_module_reset(ViewModule *vm)
{
    vm->vtable->reset(vm->module);
}

static inline void view_module_destroy(ViewModule *vm)
{
    vm->vtable->destroy(vm->module);
}

#endif
