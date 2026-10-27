#pragma once

#include <mutex>

std::recursive_mutex &berry_runtime_mutex();
void berry_runtime_lock();
void berry_runtime_unlock();
