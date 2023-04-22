#pragma once
#include "EditPluginModel.g.h"

#include "PluginModel.h"

namespace winrt::YtFlowApp::implementation
{
    struct EditPluginModel : EditPluginModelT<EditPluginModel>
    {
        EditPluginModel()
        {
        }
        EditPluginModel(com_ptr<PluginModel> plugin, bool isEntry) : m_plugin(std::move(plugin)), m_isEntry(isEntry)
        {
        }

        YtFlowApp::PluginModel Plugin();
        bool IsEntry();
        void IsEntry(bool value);
        bool IsNotEntry();
        bool IsDirty();
        void IsDirty(bool value);
        bool HasNamingConflict();
        void HasNamingConflict(bool value);
        YtFlowApp::IPluginEditorParam EditorParam();
        void EditorParam(YtFlowApp::IPluginEditorParam const &value);
        event_token PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(winrt::event_token const &token) noexcept;

      private:
        com_ptr<PluginModel> m_plugin{nullptr};
        bool m_isEntry{false};
        bool m_isDirty{false};
        bool m_hasNamingConflict{false};
        YtFlowApp::IPluginEditorParam m_editorParam;
        winrt::event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct EditPluginModel : EditPluginModelT<EditPluginModel, implementation::EditPluginModel>
    {
    };
}
