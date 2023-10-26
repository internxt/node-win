#include "stdafx.h"
#include <iostream>
#include <vector>
#include <FolderEvent.h>
#include <node_api.h>
#include <deque>

class ICallback {
    const size_t max_size_buffer = 10;
    public:
        virtual void update(napi_env env) = 0;
};