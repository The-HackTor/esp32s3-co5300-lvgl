#ifndef SCENE_MACROS_H
#define SCENE_MACROS_H

/*
 * X-macro system for scene registration.
 *
 * Usage: Each app defines a scene_config.h with lines like:
 *   ADD_SCENE(nfc, main,       Main)
 *   ADD_SCENE(nfc, read,       Read)
 *   ADD_SCENE(nfc, card_info,  CardInfo)
 *
 * Then include the appropriate macro blocks below to generate:
 *   - Scene enum
 *   - Forward declarations
 *   - Handler arrays + SceneManagerHandlers struct
 */

/* --- Generate scene ID enum --- */
#define SCENE_ENUM_GENERATE(prefix, config_file)         \
    enum {                                                \
        _SCENE_ENUM_FIRST_##prefix = -1,                 \
        config_file                                       \
        prefix##_SCENE_COUNT                              \
    };

/* When included for enum generation, ADD_SCENE expands to an enum value */
#define ADD_SCENE_ENUM(prefix, name, id) prefix##_SCENE_##id,

/* --- Generate forward declarations --- */
#define ADD_SCENE_DECL(prefix, name, id)                                   \
    void prefix##_scene_##name##_on_enter(void *ctx);                      \
    bool prefix##_scene_##name##_on_event(void *ctx, SceneEvent event);    \
    void prefix##_scene_##name##_on_exit(void *ctx);

/* --- Generate handler arrays and SceneManagerHandlers struct --- */
#define SCENE_HANDLERS_GENERATE(prefix, config_file)                        \
    static const SceneOnEnterCb prefix##_scene_on_enter[] = { config_file };\
    static const SceneOnEventCb prefix##_scene_on_event[] = { config_file };\
    static const SceneOnExitCb  prefix##_scene_on_exit[]  = { config_file };\
    static const SceneManagerHandlers prefix##_scene_handlers = {           \
        .on_enter  = prefix##_scene_on_enter,                               \
        .on_event  = prefix##_scene_on_event,                               \
        .on_exit   = prefix##_scene_on_exit,                                \
        .scene_num = prefix##_SCENE_COUNT,                                  \
    };

/* When included for on_enter array generation */
#define ADD_SCENE_ON_ENTER(prefix, name, id) \
    [prefix##_SCENE_##id] = prefix##_scene_##name##_on_enter,

/* When included for on_event array generation */
#define ADD_SCENE_ON_EVENT(prefix, name, id) \
    [prefix##_SCENE_##id] = prefix##_scene_##name##_on_event,

/* When included for on_exit array generation */
#define ADD_SCENE_ON_EXIT(prefix, name, id) \
    [prefix##_SCENE_##id] = prefix##_scene_##name##_on_exit,

#endif
