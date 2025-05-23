#include "RenameTransferContext.h"

struct CfRenameKeyLess {
    bool operator()(const CF_TRANSFER_KEY &a, const CF_TRANSFER_KEY &b) const {
        return a.QuadPart < b.QuadPart;
    }
};

static std::map<CF_TRANSFER_KEY, std::shared_ptr<RenameContext>, CfRenameKeyLess> g_renameContextMap;
static std::mutex g_renameContextMapMutex;

std::shared_ptr<RenameContext> GetOrCreateRenameContext(
    CF_CONNECTION_KEY connKey,
    CF_TRANSFER_KEY transferKey)
{
    std::lock_guard<std::mutex> lock(g_renameContextMapMutex);
    auto it = g_renameContextMap.find(transferKey);
    if (it != g_renameContextMap.end())
        return it->second;
    auto ctx = std::make_shared<RenameContext>();
    ctx->connectionKey = connKey;
    ctx->transferKey = transferKey;
    g_renameContextMap[transferKey] = ctx;
    return ctx;
}

void RemoveRenameContext(CF_TRANSFER_KEY transferKey)
{
    std::lock_guard<std::mutex> lock(g_renameContextMapMutex);
    g_renameContextMap.erase(transferKey);
}
