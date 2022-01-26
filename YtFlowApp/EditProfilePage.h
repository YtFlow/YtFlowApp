#pragma once

#include "EditProfilePage.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct EditProfilePage : EditProfilePageT<EditProfilePage>
    {
        EditProfilePage();
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct EditProfilePage : EditProfilePageT<EditProfilePage, implementation::EditProfilePage>
    {
    };
}
