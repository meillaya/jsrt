#ifndef JSRT_ENV_H
#define JSRT_ENV_H

#include <stdbool.h>

#include "jsrt/value.h"

typedef struct EnvEntry {
    char *name;
    Value value;
    bool is_const;
    struct EnvEntry *next;
} EnvEntry;

typedef struct Env {
    GcHeader gc;
    struct Env *parent;
    EnvEntry *head;
    AstUnit *units;
    bool has_this;
    Value this_value;
} Env;

typedef enum {
    ENV_ASSIGN_OK,
    ENV_ASSIGN_UNDEFINED,
    ENV_ASSIGN_CONST
} EnvAssignResult;

Env *env_create(Env *parent);
void env_destroy(Env *env);
void env_destroy_all(void);
void env_attach_unit(Env *env, AstUnit *unit);
void env_release_units(Env *env);
bool env_define(Env *env, const char *name, Value value, bool is_const);
EnvAssignResult env_assign(Env *env, const char *name, Value value);
bool env_get(const Env *env, const char *name, Value *out_value);
void env_set_this(Env *env, Value value);
bool env_get_this(const Env *env, Value *out_value);

void env_mark_reachable(Env *env);

#endif
