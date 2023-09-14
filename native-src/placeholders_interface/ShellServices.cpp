#include "stdafx.h"

#include "ClassFactory.h"
#include "ThumbnailProvider.h"

namespace
{
    template<typename T>
    DWORD make_and_register_class_object()
    {
        DWORD cookie;
        auto factory = winrt::make<ClassFactory<T>>();
        winrt::check_hresult(CoRegisterClassObject(__uuidof(T), factory.get(), CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &cookie));
        return cookie;
    }
}

void ShellServices::InitAndStartServiceTask()
{
    auto task = std::thread([]()
    {
        winrt::init_apartment(winrt::apartment_type::single_threaded);

        make_and_register_class_object<ThumbnailProvider>();

        winrt::handle dummyEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr));
        if (!dummyEvent)
        {
            winrt::throw_last_error();
        }
        DWORD index;
        HANDLE temp = dummyEvent.get();
        CoWaitForMultipleHandles(COWAIT_DISPATCH_CALLS, INFINITE, 1, &temp, &index);
    });
    task.detach();
}
