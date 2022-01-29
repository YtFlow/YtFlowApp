#pragma once
#include "EditPluginModel.g.h"

#include "PluginModel.h"

namespace winrt::YtFlowApp::implementation
{
    struct EditPluginModel : EditPluginModelT<EditPluginModel>
    {
        EditPluginModel() : m_isEntry(false), m_isDirty(false)
        {
        }
        EditPluginModel(com_ptr<PluginModel> plugin, bool isEntry)
            : m_isEntry(isEntry), m_isDirty(false), m_plugin(plugin)
        {
        }

        YtFlowApp::PluginModel Plugin();
        bool IsEntry();
        void IsEntry(bool value);
        bool IsNotEntry();
        bool IsDirty();
        void IsDirty(bool value);
        Windows::Foundation::IStringable EditorParam();
        void EditorParam(winrt::Windows::Foundation::IStringable const &value);
        event_token PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(winrt::event_token const &token) noexcept;

      private:
        com_ptr<PluginModel> m_plugin{nullptr};
        bool m_isEntry{false};
        bool m_isDirty{false};
        Windows::Foundation::IStringable m_editorParam;
        winrt::event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct EditPluginModel : EditPluginModelT<EditPluginModel, implementation::EditPluginModel>
    {
    };
}
