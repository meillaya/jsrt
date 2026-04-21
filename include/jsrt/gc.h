#ifndef JSRT_GC_H
#define JSRT_GC_H

#include <stdbool.h>
#include <stddef.h>

struct Env;
struct Value;

typedef enum {
    GC_OBJECT_ENV,
    GC_OBJECT_FUNCTION,
    GC_OBJECT_ARRAY,
    GC_OBJECT_OBJECT
} GcObjectKind;

typedef struct GcHeader {
    GcObjectKind kind;
    bool marked;
    struct GcHeader *next;
} GcHeader;

typedef enum {
    GC_TEMP_ROOT_VALUE,
    GC_TEMP_ROOT_ENV
} GcTempRootKind;

typedef struct GcTempRoot {
    GcTempRootKind kind;
    union {
        const struct Value *value;
        struct Env *env;
    } as;
    struct GcTempRoot *previous;
} GcTempRoot;

typedef struct {
    size_t collections_total;
    size_t live_envs;
    size_t live_functions;
    size_t live_arrays;
    size_t live_objects;
    size_t swept_envs;
    size_t swept_functions;
    size_t swept_arrays;
    size_t swept_objects;
    size_t temp_roots_current;
    size_t temp_roots_high_water;
    char last_reason[32];
} GcStatsSnapshot;

void gc_register_object(GcHeader *header, GcObjectKind kind);
void gc_temp_root_push_value(GcTempRoot *root, const struct Value *value);
void gc_temp_root_push_env(GcTempRoot *root, struct Env *env);
void gc_temp_root_pop(GcTempRoot *root);

void gc_collect(struct Env *root_env, const struct Value *root_value);
void gc_collect_with_reason(struct Env *root_env, const struct Value *root_value, const char *reason);
void gc_reset_stats(void);
void gc_get_stats(GcStatsSnapshot *out_snapshot);
void gc_set_trace_enabled(bool enabled);

size_t gc_live_env_count(void);
size_t gc_live_function_count(void);
size_t gc_live_array_count(void);
size_t gc_live_object_count(void);

#endif
