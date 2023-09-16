#pragma once
#include "ProfileModel.g.h"

#include "CoreFfi.h"

namespace winrt::YtFlowApp::implementation
{
    struct ProfileModel : ProfileModelT<ProfileModel>
    {
        ProfileModel() = default;
        ProfileModel(FfiProfile const &profile)
            : m_id(profile.id), m_name(to_hstring(profile.name)), m_locale(to_hstring(profile.locale))
        {
        }

        event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(event_token const &token) noexcept;
        uint32_t Id();
        void Id(uint32_t const &);
        hstring Name();
        void Name(hstring const &);
        hstring Locale();
        void Locale(hstring const &);

      private:
        event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
        uint32_t m_id{};
        hstring m_name;
        hstring m_locale;
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct ProfileModel : ProfileModelT<ProfileModel, implementation::ProfileModel>
    {
    };
}
