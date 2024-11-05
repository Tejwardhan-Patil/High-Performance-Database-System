%module cache

%{
#include "lfu_cache.h"
#include "lru_cache.h"
#include "write_back_cache.h"
#include "write_through_cache.h"
%}

%include "lfu_cache.h"
%include "lru_cache.h"
%include "write_back_cache.h"
%include "write_through_cache.h"