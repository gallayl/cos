#pragma once

#include <mutex>

struct bvm;

std::recursive_mutex &berry_runtime_mutex();
void berry_runtime_lock();
void berry_runtime_unlock();
struct bvm *berry_runtime_get_vm(); // R24: shared VM pointer for serialized callback bridge (use under runtime lock)
