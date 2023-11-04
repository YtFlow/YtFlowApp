#pragma once

#include "ResourceModel.g.h"

#include "CoreFfi.h"

namespace winrt::YtFlowApp::implementation
{
    struct ResourceModel : ResourceModelT<ResourceModel>
    {
        ResourceModel() = default;
        ResourceModel(FfiResource const &resource)
            : m_id(resource.id), m_key(to_hstring(resource.key)), m_type(resource.type),
              m_localFile(resource.local_file), m_remoteType(resource.remote_type)
        {
        }

        event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(event_token const &token) noexcept;

      private:
        uint32_t m_id{};
        hstring m_key;
        std::string m_type;
        std::string m_localFile;
        std::string m_remoteType;
        event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}

namespace winrt::YtFlowApp::factory_implementation
{
    struct ResourceModel : ResourceModelT<ResourceModel, implementation::ResourceModel>
    {
    };
}
