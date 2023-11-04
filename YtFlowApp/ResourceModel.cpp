#include "pch.h"
#include "ResourceModel.h"
#if __has_include("ResourceModel.g.cpp")
#include "ResourceModel.g.cpp"
#endif

namespace winrt::YtFlowApp::implementation
{
    event_token ResourceModel::PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler)
    {
        return m_propertyChanged.add(handler);
    }
    void ResourceModel::PropertyChanged(event_token const &token) noexcept
    {
        m_propertyChanged.remove(token);
    }

}
