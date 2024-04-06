#pragma once
#include "AssetModel.g.h"

namespace winrt::YtFlowApp::implementation
{
    using Windows::Foundation::Collections::IObservableVector;

    struct AssetModel : AssetModelT<AssetModel>
    {
        AssetModel() = default;

        IObservableVector<YtFlowApp::ProxyGroupModel> ProxyGroups();
        void ProxyGroups(IObservableVector<YtFlowApp::ProxyGroupModel> const &value);
        bool IsProxyGroupsEmpty() const;
        YtFlowApp::ProxyGroupModel CurrentProxyGroupModel() const;
        void CurrentProxyGroupModel(YtFlowApp::ProxyGroupModel const &value);

        event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(event_token const &token) noexcept;

      private:
        IObservableVector<YtFlowApp::ProxyGroupModel> m_proxyGroups;
        event_token m_proxyGroupsChangeHandle;
        event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
        YtFlowApp::ProxyGroupModel m_currentProxyModel{nullptr};
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct AssetModel : AssetModelT<AssetModel, implementation::AssetModel>
    {
    };
}
