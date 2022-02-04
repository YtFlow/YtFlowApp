#include "pch.h"
#include "EditPluginModel.h"
#include "EditPluginModel.g.cpp"

using namespace winrt;
using namespace Windows::Foundation;

namespace winrt::YtFlowApp::implementation
{
    YtFlowApp::PluginModel EditPluginModel::Plugin()
    {
        return *m_plugin;
    }
    bool EditPluginModel::IsEntry()
    {
        return m_isEntry;
    }
    void EditPluginModel::IsEntry(bool value)
    {
        m_isEntry = value;
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"IsEntry"});
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"IsNotEntry"});
    }
    bool EditPluginModel::IsNotEntry()
    {
        return !m_isEntry;
    }
    bool EditPluginModel::IsDirty()
    {
        return m_isDirty;
    }
    void EditPluginModel::IsDirty(bool value)
    {
        m_isDirty = value;
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"IsDirty"});
    }
    bool EditPluginModel::HasNamingConflict()
    {
        return m_hasNamingConflict;
    }
    void EditPluginModel::HasNamingConflict(bool value)
    {
        m_hasNamingConflict = value;
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs(L"HasNamingConflict"));
    }
    YtFlowApp::IPluginEditorParam EditPluginModel::EditorParam()
    {
        return m_editorParam;
    }
    void EditPluginModel::EditorParam(IPluginEditorParam const &value)
    {
        m_editorParam = value;
        m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{L"EditorParam"});
    }
    event_token EditPluginModel::PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const &handler)
    {
        return m_propertyChanged.add(handler);
    }
    void EditPluginModel::PropertyChanged(event_token const &token) noexcept
    {
        m_propertyChanged.remove(token);
    }
}
