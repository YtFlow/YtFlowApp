#pragma once
#include "ProfileModel.g.h"

#include "CoreFfi.h"

namespace winrt::YtFlowApp::implementation
{
    struct ProfileModel : ProfileModelT<ProfileModel>
    {
        ProfileModel() = default;
        ProfileModel(FfiProfile const &profile) : m_name(winrt::to_hstring(profile.name)), m_id(profile.id)
        {
        }

        winrt::event_token PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(winrt::event_token const &token) noexcept;
        hstring Name();
        void Name(hstring const &);
        uint32_t Id();
        void Id(uint32_t const &);

      private:
        winrt::event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
        hstring m_name;
        uint32_t m_id;
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct ProfileModel : ProfileModelT<ProfileModel, implementation::ProfileModel>
    {
    };
}
