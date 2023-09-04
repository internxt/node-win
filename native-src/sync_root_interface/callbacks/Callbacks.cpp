#include <Callbacks.h>
#include <string>

void CallbackHandler::RegisterThreadSafeCallbacks(CallbackContext* context) {
    RegisterThreadSafeNotifyDeleteCallback("NotifyDeleteThreadSafe", context);
}