#pragma once
#include "PluginModel.g.h"

#include "CoreFfi.h"

namespace winrt::YtFlowApp::implementation
{
    struct PluginModel : PluginModelT<PluginModel>
    {
        PluginModel(FfiPlugin const &plugin, uint32_t profileId)
            : OriginalPlugin(plugin), m_id(plugin.id), m_profileId(profileId), m_name(to_hstring(plugin.name)),
              m_desc(to_hstring(plugin.desc)), m_plugin(to_hstring(plugin.plugin)),
              m_pluginVersion(plugin.plugin_version), m_param(plugin.param)
        {
        }

        uint32_t Id();
        uint32_t ProfileId();
        hstring Name();
        void Name(hstring const &value);
        hstring Desc();
        void Desc(hstring const &value);
        hstring Plugin();
        void Plugin(hstring const &value);
        uint16_t PluginVersion();
        void PluginVersion(uint16_t value);
        com_array<uint8_t> Param();
        void Param(array_view<uint8_t const> value);

        FfiPlugin OriginalPlugin;

        event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(event_token const &token) noexcept;

        FfiPluginVerifyResult Verify() const &;
        std::set<hstring> GetDependencyPlugins() const &;
        void SetAsEntry() const &;
        void UnsetAsEntry() const &;
        void Update() const &;

      private:
        uint32_t m_id;
        uint32_t m_profileId;
        hstring m_name;
        hstring m_desc;
        hstring m_plugin;
        uint16_t m_pluginVersion;
        std::vector<uint8_t> m_param;
        event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
/*
namespace winrt::YtFlowApp::factory_implementation
{
    struct PluginModel : PluginModelT<PluginModel, implementation::PluginModel>
    {
    };
}
*/
