#ifndef BERRY_CONF_HOST_VALIDATION_H
#define BERRY_CONF_HOST_VALIDATION_H

/*
 * Berry host-validation config derived from components/berry_lang/src/berry_conf.h.
 * Matches firmware-relevant compiler behavior (especially non-strict undeclared
 * globals, since COS scripts rely on runtime-registered globals like cos_import,
 * action, log, console, fs, net, ui). Host CLI needs bytecode saver/loader and
 * file system support for `berry -c ... -o ...`.
 */

#define BE_INTGER_TYPE 1
#define BE_USE_SINGLE_FLOAT 1
#define BE_BYTES_MAX_SIZE (16 * 1024)
#define BE_USE_PRECOMPILED_OBJECT 1

#define BE_DEBUG_SOURCE_FILE 0
#define BE_DEBUG_RUNTIME_INFO 2
#define BE_DEBUG_VAR_INFO 0
#define BE_USE_PERF_COUNTERS 0
#define BE_VM_OBSERVABILITY_SAMPLING 20

#define BE_STACK_TOTAL_MAX 4000
#define BE_STACK_FREE_MIN 10
#define BE_STACK_START 40
#define BE_MAX_PARSER_DEPTH 25

#define BE_CONST_SEARCH_SIZE 50
#define BE_USE_STR_LENGTH_HASH 1
#define BE_USE_STR_HASH_CACHE 0
#define BE_USE_FILE_SYSTEM 1
#define BE_USE_SCRIPT_COMPILER 1

#define BE_USE_BYTECODE_SAVER 1
#define BE_USE_BYTECODE_LOADER 1
#define BE_USE_SHARED_LIB 0
#define BE_USE_OVERLOAD_HASH 0
#define BE_USE_DEBUG_HOOK 0
#define BE_USE_DEBUG_GC 0
#define BE_USE_DEBUG_STACK 0
#define BE_USE_MEM_ALIGNED 0

#define BE_USE_XXX_RESERVED 0

#define BE_USE_STRING_MODULE 1
#define BE_USE_JSON_MODULE 1
#define BE_USE_MATH_MODULE 1
#define BE_USE_TIME_MODULE 0
#define BE_USE_OS_MODULE 0
#define BE_USE_GLOBAL_MODULE 1
#define BE_USE_SYS_MODULE 0
#define BE_USE_DEBUG_MODULE 0
#define BE_USE_GC_MODULE 1
#define BE_USE_SOLIDIFY_MODULE 0
#define BE_USE_INTROSPECT_MODULE 1
#define BE_USE_STRICT_MODULE 0

#define BE_EXPLICIT_ABORT abort
#define BE_EXPLICIT_EXIT exit
#define BE_EXPLICIT_MALLOC malloc
#define BE_EXPLICIT_FREE free
#define BE_EXPLICIT_REALLOC realloc

#endif
