#include "TransferContext.h"

struct CfTransferKeyLess {
    bool operator()(const CF_TRANSFER_KEY &a, const CF_TRANSFER_KEY &b) const {
        return a.QuadPart < b.QuadPart;
    }
};

static std::map<CF_TRANSFER_KEY, std::shared_ptr<TransferContext>, CfTransferKeyLess> g_transferContextMap;

static std::mutex g_contextMapMutex;

std::shared_ptr<TransferContext> GetOrCreateTransferContext(CF_CONNECTION_KEY connKey, CF_TRANSFER_KEY transferKey) {
    std::lock_guard<std::mutex> lock(g_contextMapMutex);

    auto it = g_transferContextMap.find(transferKey);
    if (it != g_transferContextMap.end()) {
        return it->second;
    }
    
    auto ctx = std::make_shared<TransferContext>();
    ctx->connectionKey = connKey;
    ctx->transferKey = transferKey;
    g_transferContextMap[transferKey] = ctx;
    return ctx;
}

void RemoveTransferContext(CF_TRANSFER_KEY transferKey)  {
    std::lock_guard<std::mutex> lock(g_contextMapMutex);
    g_transferContextMap.erase(transferKey);
}
