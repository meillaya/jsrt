#include "jsrt/gc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsrt/env.h"
#include "jsrt/value.h"

static GcHeader *gc_registry_head = NULL;
static GcTempRoot *gc_temp_roots = NULL;
static GcStatsSnapshot gc_stats = {0};
static bool gc_trace_enabled = false;
static size_t gc_temp_root_count = 0;

static size_t gc_live_count(GcObjectKind kind);

static void gc_refresh_live_stats(void) {
    gc_stats.live_envs = gc_live_count(GC_OBJECT_ENV);
    gc_stats.live_functions = gc_live_count(GC_OBJECT_FUNCTION);
    gc_stats.live_arrays = gc_live_count(GC_OBJECT_ARRAY);
    gc_stats.live_objects = gc_live_count(GC_OBJECT_OBJECT);
    gc_stats.temp_roots_current = gc_temp_root_count;
}

static void gc_record_temp_root_push(void) {
    gc_temp_root_count += 1U;
    gc_stats.temp_roots_current = gc_temp_root_count;
    if (gc_temp_root_count > gc_stats.temp_roots_high_water) {
        gc_stats.temp_roots_high_water = gc_temp_root_count;
    }
}

static void gc_set_last_reason(const char *reason) {
    const char *source = (reason != NULL && reason[0] != '\0') ? reason : "unspecified";

    snprintf(gc_stats.last_reason, sizeof(gc_stats.last_reason), "%s", source);
}

static void gc_trace_collection(void) {
    if (!gc_trace_enabled) {
        return;
    }

    fprintf(
        stderr,
        "[gc] reason=%s collections=%zu live(env=%zu,function=%zu,array=%zu,object=%zu) "
        "swept(env=%zu,function=%zu,array=%zu,object=%zu) temp_roots(current=%zu,high_water=%zu)\n",
        gc_stats.last_reason,
        gc_stats.collections_total,
        gc_stats.live_envs,
        gc_stats.live_functions,
        gc_stats.live_arrays,
        gc_stats.live_objects,
        gc_stats.swept_envs,
        gc_stats.swept_functions,
        gc_stats.swept_arrays,
        gc_stats.swept_objects,
        gc_stats.temp_roots_current,
        gc_stats.temp_roots_high_water
    );
}

void gc_register_object(GcHeader *header, GcObjectKind kind) {
    header->kind = kind;
    header->marked = false;
    header->next = gc_registry_head;
    gc_registry_head = header;
}

void gc_temp_root_push_value(GcTempRoot *root, const struct Value *value) {
    root->kind = GC_TEMP_ROOT_VALUE;
    root->as.value = value;
    root->previous = gc_temp_roots;
    gc_temp_roots = root;
    gc_record_temp_root_push();
}

void gc_temp_root_push_env(GcTempRoot *root, struct Env *env) {
    root->kind = GC_TEMP_ROOT_ENV;
    root->as.env = env;
    root->previous = gc_temp_roots;
    gc_temp_roots = root;
    gc_record_temp_root_push();
}

void gc_temp_root_pop(GcTempRoot *root) {
    GcTempRoot **cursor = &gc_temp_roots;

    while (*cursor != NULL) {
        if (*cursor == root) {
            *cursor = root->previous;
            if (gc_temp_root_count > 0U) {
                gc_temp_root_count -= 1U;
            }
            gc_stats.temp_roots_current = gc_temp_root_count;
            break;
        }
        cursor = &(*cursor)->previous;
    }
    root->previous = NULL;
}

static void gc_clear_marks(void) {
    GcHeader *cursor = gc_registry_head;

    while (cursor != NULL) {
        cursor->marked = false;
        cursor = cursor->next;
    }
}

static void gc_mark_temp_roots(void) {
    const GcTempRoot *cursor = gc_temp_roots;

    while (cursor != NULL) {
        if (cursor->kind == GC_TEMP_ROOT_VALUE) {
            if (cursor->as.value != NULL) {
                value_mark_reachable(*cursor->as.value);
            }
        } else if (cursor->kind == GC_TEMP_ROOT_ENV && cursor->as.env != NULL) {
            env_mark_reachable(cursor->as.env);
        }
        cursor = cursor->previous;
    }
}

static void gc_sweep_env(Env *env) {
    EnvEntry *entry = env->head;

    while (entry != NULL) {
        EnvEntry *next = entry->next;
        free(entry->name);
        value_release_reference(entry->value);
        free(entry);
        entry = next;
    }

    if (env->has_this) {
        value_release_reference(env->this_value);
    }
    free(env);
}

static void gc_sweep_function(JsFunction *function) {
    size_t i;
    for (i = 0; i < function->capture_count; ++i) {
        value_release_reference(function->captures[i]);
    }
    free(function->captures);
    free(function);
}

static void gc_sweep_array(JsArray *array) {
    size_t i;

    for (i = 0; i < array->count; ++i) {
        value_release_reference(array->items[i]);
    }
    free(array->items);
    free(array);
}

static void gc_sweep_object(JsObject *object) {
    size_t i;

    for (i = 0; i < object->count; ++i) {
        free(object->items[i].key);
        value_release_reference(object->items[i].value);
    }
    free(object->items);
    free(object);
}

void gc_collect(struct Env *root_env, const struct Value *root_value) {
    gc_collect_with_reason(root_env, root_value, NULL);
}

void gc_collect_with_reason(struct Env *root_env, const struct Value *root_value, const char *reason) {
    GcHeader **cursor;

    gc_clear_marks();
    if (root_env != NULL) {
        env_mark_reachable(root_env);
    }
    if (root_value != NULL) {
        value_mark_reachable(*root_value);
    }
    gc_mark_temp_roots();

    gc_stats.swept_envs = 0U;
    gc_stats.swept_functions = 0U;
    gc_stats.swept_arrays = 0U;
    gc_stats.swept_objects = 0U;
    gc_set_last_reason(reason);

    cursor = &gc_registry_head;
    while (*cursor != NULL) {
        GcHeader *header = *cursor;

        if (header->marked) {
            cursor = &header->next;
            continue;
        }

        *cursor = header->next;
        switch (header->kind) {
            case GC_OBJECT_ENV:
                gc_stats.swept_envs += 1U;
                gc_sweep_env((Env *)header);
                break;
            case GC_OBJECT_FUNCTION:
                gc_stats.swept_functions += 1U;
                gc_sweep_function((JsFunction *)header);
                break;
            case GC_OBJECT_ARRAY:
                gc_stats.swept_arrays += 1U;
                gc_sweep_array((JsArray *)header);
                break;
            case GC_OBJECT_OBJECT:
                gc_stats.swept_objects += 1U;
                gc_sweep_object((JsObject *)header);
                break;
        }
    }

    gc_stats.collections_total += 1U;
    gc_refresh_live_stats();
    gc_trace_collection();
}

void gc_reset_stats(void) {
    memset(&gc_stats, 0, sizeof(gc_stats));
    gc_stats.temp_roots_current = gc_temp_root_count;
}

void gc_get_stats(GcStatsSnapshot *out_snapshot) {
    if (out_snapshot == NULL) {
        return;
    }

    gc_refresh_live_stats();
    *out_snapshot = gc_stats;
}

void gc_set_trace_enabled(bool enabled) {
    gc_trace_enabled = enabled;
}

static size_t gc_live_count(GcObjectKind kind) {
    const GcHeader *cursor = gc_registry_head;
    size_t count = 0;

    while (cursor != NULL) {
        if (cursor->kind == kind) {
            count += 1U;
        }
        cursor = cursor->next;
    }

    return count;
}

size_t gc_live_env_count(void) {
    return gc_live_count(GC_OBJECT_ENV);
}

size_t gc_live_function_count(void) {
    return gc_live_count(GC_OBJECT_FUNCTION);
}

size_t gc_live_array_count(void) {
    return gc_live_count(GC_OBJECT_ARRAY);
}

size_t gc_live_object_count(void) {
    return gc_live_count(GC_OBJECT_OBJECT);
}
