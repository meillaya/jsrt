#include "jsrt/env.h"

#include <stdlib.h>
#include <string.h>

#include "jsrt/gc.h"
#include "jsrt/memory.h"

Env *env_create(Env *parent) {
    Env *env = jsrt_malloc(sizeof(*env));
    gc_register_object(&env->gc, GC_OBJECT_ENV);
    env->parent = parent;
    env->head = NULL;
    env->has_this = false;
    env->this_value = value_null();
    return env;
}

void env_destroy(Env *env) {
    if (env == NULL) {
        return;
    }
}

void env_destroy_all(void) {
    gc_collect_with_reason(NULL, NULL, "env-destroy-all");
}

bool env_define(Env *env, const char *name, Value value, bool is_const) {
    EnvEntry *cursor = env->head;
    while (cursor != NULL) {
        if (strcmp(cursor->name, name) == 0) {
            return false;
        }
        cursor = cursor->next;
    }

    EnvEntry *entry = jsrt_malloc(sizeof(*entry));
    entry->name = jsrt_strdup(name);
    entry->value = value_clone(value);
    entry->is_const = is_const;
    entry->next = env->head;
    env->head = entry;
    return true;
}

EnvAssignResult env_assign(Env *env, const char *name, Value value) {
    Env *current = env;
    while (current != NULL) {
        EnvEntry *entry = current->head;
        while (entry != NULL) {
            if (strcmp(entry->name, name) == 0) {
                if (entry->is_const) {
                    return ENV_ASSIGN_CONST;
                }
                value_free(entry->value);
                entry->value = value_clone(value);
                return ENV_ASSIGN_OK;
            }
            entry = entry->next;
        }
        current = current->parent;
    }
    return ENV_ASSIGN_UNDEFINED;
}

bool env_get(const Env *env, const char *name, Value *out_value) {
    const Env *current = env;
    while (current != NULL) {
        const EnvEntry *entry = current->head;
        while (entry != NULL) {
            if (strcmp(entry->name, name) == 0) {
                *out_value = value_clone(entry->value);
                return true;
            }
            entry = entry->next;
        }
        current = current->parent;
    }
    return false;
}

void env_set_this(Env *env, Value value) {
    if (env->has_this) {
        value_free(env->this_value);
    }
    env->has_this = true;
    env->this_value = value_clone(value);
}

bool env_get_this(const Env *env, Value *out_value) {
    const Env *current = env;
    while (current != NULL) {
        if (current->has_this) {
            *out_value = value_clone(current->this_value);
            return true;
        }
        current = current->parent;
    }
    return false;
}

void env_mark_reachable(Env *env) {
    EnvEntry *entry;

    if (env == NULL || env->gc.marked) {
        return;
    }

    env->gc.marked = true;
    if (env->has_this) {
        value_mark_reachable(env->this_value);
    }
    entry = env->head;
    while (entry != NULL) {
        value_mark_reachable(entry->value);
        entry = entry->next;
    }
    env_mark_reachable(env->parent);
}
