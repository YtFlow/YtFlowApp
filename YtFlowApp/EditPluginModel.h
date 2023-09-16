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
        IPluginEditorParam EditorParam();
        void EditorParam(IPluginEditorParam const &value);
        event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler);
        void PropertyChanged(event_token const &token) noexcept;

      private:
        com_ptr<PluginModel> m_plugin{nullptr};
        bool m_isEntry{false};
        bool m_isDirty{false};
        bool m_hasNamingConflict{false};
        IPluginEditorParam m_editorParam;
        event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
namespace winrt::YtFlowApp::factory_implementation
{
    struct EditPluginModel : EditPluginModelT<EditPluginModel, implementation::EditPluginModel>
    {
    };
}
