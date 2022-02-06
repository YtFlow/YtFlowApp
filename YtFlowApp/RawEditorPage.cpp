#include "pch.h"
#include "RawEditorPage.h"
#if __has_include("RawEditorPage.g.cpp")
#include "RawEditorPage.g.cpp"
#endif

#include "PluginModel.h"
#include "RawEditorParam.h"

using namespace winrt;
using namespace Windows::UI::Text;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;

namespace winrt::YtFlowApp::implementation
{
    constexpr const uint8_t PARAM_EDIT_TEXT_INITED = 0b01;
    constexpr const uint8_t PARAM_EDIT_TEXT_STORED = 0b10;
    RawEditorPage::RawEditorPage()
    {
        InitializeComponent();
    }

    YtFlowApp::EditPluginModel RawEditorPage::Model()
    {
        return m_model;
    }

    void RawEditorPage::Model(YtFlowApp::EditPluginModel const &value)
    {
        m_model = value;
    }

    void RawEditorPage::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const &args)
    {
        auto model{std::move(args.Parameter().as<EditPluginModel>())};
        auto editorParam{model.EditorParam().try_as<YtFlowApp::RawEditorParam>()};
        if (editorParam == nullptr)
        {
            auto const param{model.Plugin().Param()};
            editorParam = std::move(winrt::make<RawEditorParam>(param.data(), param.size()));
            model.EditorParam(editorParam);
        }
        Bindings->StopTracking();
        Model(std::move(model));
        Bindings->Update();
        m_paramEditTextChangedStage = 0;
        ParamEdit().Document().SetText(TextSetOptions::None, editorParam.RawJson());
    }
    void RawEditorPage::OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const & /* args */)
    {
        auto const editorParam{m_model.EditorParam().as<YtFlowApp::RawEditorParam>()};
        hstring text;
        ParamEdit().Document().GetText(TextGetOptions::NoHidden, text);
        editorParam.RawJson(text);
    }
    SolidColorBrush RawEditorPage::PluginNameColor(bool hasNamingConflict)
    {
        if (hasNamingConflict)
        {
            return SolidColorBrush{Windows::UI::Colors::Red()};
        }
        else
        {
            return Application::Current()
                .Resources()
                .Lookup(box_value(L"DefaultTextForegroundThemeBrush"))
                .as<SolidColorBrush>();
        }
    }

    void RawEditorPage::ParamEdit_LostFocus(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        if (m_paramEditTextChangedStage & PARAM_EDIT_TEXT_STORED)
        {
            return;
        }
        hstring text;
        ParamEdit().Document().GetText(TextGetOptions::NoHidden, text);
        m_model.EditorParam().as<YtFlowApp::RawEditorParam>().RawJson(text);
        m_paramEditTextChangedStage |= PARAM_EDIT_TEXT_STORED;
    }

    void RawEditorPage::ParamEdit_TextChanged(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        if (m_paramEditTextChangedStage & PARAM_EDIT_TEXT_INITED)
        {
            m_model.IsDirty(true);
            m_paramEditTextChangedStage &= ~PARAM_EDIT_TEXT_STORED;
        }
        else
        {
            m_paramEditTextChangedStage |= PARAM_EDIT_TEXT_INITED | PARAM_EDIT_TEXT_STORED;
        }
    }

    void RawEditorPage::ResetButton_Click(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        auto const pluginModel{get_self<PluginModel>(m_model.Plugin())};
        auto const &original{pluginModel->OriginalPlugin};
        pluginModel->Name(to_hstring(original.name));
        pluginModel->Desc(to_hstring(original.desc));
        pluginModel->Param(com_array<uint8_t>(original.param));

        auto const param{winrt::make<RawEditorParam>(original.param.data(), original.param.size())};
        m_model.EditorParam(param);
        m_paramEditTextChangedStage = 0;
        ParamEdit().Document().SetText(TextSetOptions::None, param.RawJson());

        m_model.IsDirty(false);
    }

    fire_and_forget RawEditorPage::SaveButton_Click(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        auto const lifetime{get_strong()};
        if (m_model.HasNamingConflict())
        {
            PluginNameBox().Focus(FocusState::Programmatic);
            co_return;
        }
        auto const editorParam{get_self<RawEditorParam>(m_model.EditorParam().as<YtFlowApp::RawEditorParam>())};
        hstring text;
        ParamEdit().Document().GetText(TextGetOptions::NoHidden, text);
        editorParam->RawJson(text);
        auto const paramErrors{editorParam->CheckErrors()};
        if (paramErrors.size() > 0)
        {
            hstring errorText;
            for (auto const &err : paramErrors)
            {
                errorText = errorText + err + L"\r\n";
            }
            ValidateErrorText().Text(std::move(errorText));
            ValidateErrorFlyout().ShowAt(SaveButton());
            co_return;
        }
        editorParam->Prettify();
        ParamEdit().Document().SetText(TextSetOptions::None, editorParam->RawJson());

        auto const pluginModel{get_self<PluginModel>(m_model.Plugin())};
        auto const cbor{editorParam->ToCbor()};
        pluginModel->Param(cbor);
        try
        {
            pluginModel->Verify();
        }
        catch (FfiException ex)
        {
            ValidateErrorText().Text(to_hstring(ex.msg));
            ValidateErrorFlyout().ShowAt(SaveButton());
            co_return;
        }

        auto original{pluginModel->OriginalPlugin};

        original.name = winrt::to_string(pluginModel->Name());
        original.desc = winrt::to_string(pluginModel->Desc());
        original.param = std::vector<uint8_t>(cbor.begin(), cbor.end());
        pluginModel->OriginalPlugin = std::move(original);

        co_await resume_background();
        pluginModel->Update();
        co_await resume_foreground(Dispatcher());

        m_model.IsDirty(false);
    }

}
