#include "pch.h"
#include "AssetModel.h"
#include "AssetModel.g.cpp"

namespace winrt::YtFlowApp::implementation
{
    using Windows::UI::Xaml::Data::PropertyChangedEventArgs;

    IObservableVector<YtFlowApp::ProxyGroupModel> AssetModel::ProxyGroups()
    {
        return m_proxyGroups;
    }
    void AssetModel::ProxyGroups(IObservableVector<YtFlowApp::ProxyGroupModel> const &value)
    {
        if (m_proxyGroupsChangeHandle)
        {
            m_proxyGroups.VectorChanged(m_proxyGroupsChangeHandle);
        }
        m_proxyGroups = value;
        value.VectorChanged([weak{get_weak()}](auto const &, auto const &) {
            if (auto self{weak.get()})
            {
                self->m_propertyChanged(*self, PropertyChangedEventArgs(L"IsProxyGroupsEmpty"));
            }
        });
        m_propertyChanged(*this, PropertyChangedEventArgs(L"ProxyGroups"));
        m_propertyChanged(*this, PropertyChangedEventArgs(L"IsProxyGroupsEmpty"));
    }
    bool AssetModel::IsProxyGroupsEmpty() const
    {
        return m_proxyGroups == nullptr || m_proxyGroups.Size() == 0;
    }

    event_token AssetModel::PropertyChanged(
        Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler)
    {
        return m_propertyChanged.add(handler);
    }
    void AssetModel::PropertyChanged(event_token const &token) noexcept
    {
        m_propertyChanged.remove(token);
    }
    YtFlowApp::ProxyGroupModel AssetModel::CurrentProxyGroupModel() const
    {
        return m_currentProxyModel;
    }
    void AssetModel::CurrentProxyGroupModel(YtFlowApp::ProxyGroupModel const &value)
    {
        m_currentProxyModel = value;
        m_propertyChanged(*this, PropertyChangedEventArgs(L"CurrentProxyGroupModel"));
    }

}
