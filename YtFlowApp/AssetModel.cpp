#include "pch.h"
#include "AssetModel.h"
#include "AssetModel.g.cpp"

namespace winrt::YtFlowApp::implementation
{
    using Windows::UI::Xaml::Data::PropertyChangedEventArgs;

    IObservableVector<ProxyGroupModel> AssetModel::ProxyGroups()
    {
        return m_proxyGroups;
    }
    void AssetModel::ProxyGroups(IObservableVector<ProxyGroupModel> const &value)
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
    winrt::event_token AssetModel::PropertyChanged(
        winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler)
    {
        return m_propertyChanged.add(handler);
    }
    void AssetModel::PropertyChanged(winrt::event_token const &token) noexcept
    {
        m_propertyChanged.remove(token);
    }
    ProxyGroupModel AssetModel::CurrentProxyGroupModel() const
    {
        return m_currentProxyModel;
    }
    void AssetModel::CurrentProxyGroupModel(ProxyGroupModel const &value)
    {
        m_currentProxyModel = value;
        m_propertyChanged(*this, PropertyChangedEventArgs(L"CurrentProxyGroupModel"));
    }

}
