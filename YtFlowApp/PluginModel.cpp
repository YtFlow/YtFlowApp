#include "pch.h"
#include "PluginModel.h"
#include "PluginModel.g.cpp"

namespace winrt::YtFlowApp::implementation
{
    uint32_t PluginModel::Id()
    {
        return m_id;
    }
    uint32_t PluginModel::ProfileId()
    {
        return m_profileId;
    }
    hstring PluginModel::Name()
    {
        return m_name;
    }
    void PluginModel::Name(hstring const &value)
    {
        m_name = value;
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Name"});
    }
    hstring PluginModel::Desc()
    {
        return m_desc;
    }
    void PluginModel::Desc(hstring const &value)
    {
        m_desc = value;
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Desc"});
    }
    hstring PluginModel::Plugin()
    {
        return m_plugin;
    }
    void PluginModel::Plugin(hstring const &value)
    {
        m_plugin = value;
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Plugin"});
    }
    uint16_t PluginModel::PluginVersion()
    {
        return m_pluginVersion;
    }
    void PluginModel::PluginVersion(uint16_t value)
    {
        m_pluginVersion = value;
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"PluginVersion"});
    }
    com_array<uint8_t> PluginModel::Param()
    {
        return com_array<uint8_t>(m_param);
    }
    void PluginModel::Param(array_view<uint8_t const> value)
    {
        m_param = std::vector(value.begin(), value.end());
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"Param"});
    }
    winrt::event_token PluginModel::PropertyChanged(
        winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler)
    {
        return m_propertyChanged.add(handler);
    }
    void PluginModel::PropertyChanged(winrt::event_token const &token) noexcept
    {
        m_propertyChanged.remove(token);
    }

    FfiPluginVerifyResult PluginModel::Verify() const &
    {
        return FfiPlugin::verify(winrt::to_string(m_plugin).data(), m_pluginVersion, m_param.data(), m_param.size());
    }
    std::set<hstring> PluginModel::GetDependencyPlugins() const &
    {
        std::set<hstring> ret;
        auto const verifyRes{Verify()};
        for (auto const &req : verifyRes.required)
        {
            auto dotPos = req.descriptor.find('.', 0);
            if (dotPos == std::string::npos)
            {
                dotPos = req.descriptor.size();
            }
            ret.insert(to_hstring(std::string_view(req.descriptor.data(), dotPos)));
        }
        return ret;
    }
    void PluginModel::SetAsEntry() const &
    {
        auto conn{FfiDbInstance.Connect()};
        try
        {
            conn.SetPluginAsEntry(m_id, m_profileId);
        }
        catch (FfiException)
        {
            // Ignore duplicated entry error
        }
    }
    void PluginModel::UnsetAsEntry() const &
    {
        auto conn{FfiDbInstance.Connect()};
        try
        {
            conn.UnsetPluginAsEntry(m_id, m_profileId);
        }
        catch (FfiException)
        {
            // Ignore no entry error
        }
    }
    void PluginModel::Update() const &
    {
        auto conn{FfiDbInstance.Connect()};
        conn.UpdatePlugin(m_id, m_profileId, winrt::to_string(m_name).data(), winrt::to_string(m_desc).data(),
                          winrt::to_string(m_plugin).data(), m_pluginVersion, m_param.data(), m_param.size());
    }
}
