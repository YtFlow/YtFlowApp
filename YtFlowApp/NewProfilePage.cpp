#include "pch.h"
#include "NewProfilePage.h"
#if __has_include("NewProfilePage.g.cpp")
#include "NewProfilePage.g.cpp"
#endif

#include "CoreFfi.h"

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;

namespace winrt::YtFlowApp::implementation
{
    NewProfilePage::NewProfilePage()
    {
        InitializeComponent();
    }

    fire_and_forget NewProfilePage::SaveButton_Click(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        const auto newProfileName{to_string(NewProfileNameText().Text())};
        if (newProfileName.empty())
        {
            NewProfileNameText().Focus(FocusState::Programmatic);
            co_return;
        }

        SaveButton().IsEnabled(false);
        const auto lifetime{get_strong()};
        bool nameDuplicated{false};
        co_await resume_background();

        const auto conn{FfiDbInstance.Connect()};
        for (const auto &profile : conn.GetProfiles())
        {
            if (profile.name == newProfileName)
            {
                nameDuplicated = true;
                break;
            }
        }
        if (!nameDuplicated)
        {
            conn.CreateProfile(newProfileName.data(), "en-US");
        }

        co_await resume_foreground(Dispatcher());
        if (nameDuplicated)
        {
            NewProfileNameText().Foreground(Media::SolidColorBrush{Windows::UI::Colors::Red()});
            SaveButton().IsEnabled(true);
        }
        else
        {
            Frame().GoBack();
        }
    }

    void NewProfilePage::OutboundTypeButton_Checked(IInspectable const &sender, RoutedEventArgs const & /* e */)
    {
        const auto &btn = sender.as<RadioButton>();
        auto choice{btn.Tag().as<hstring>()};
        OutputDebugString(choice.data());
    }

    void NewProfilePage::Page_Loaded(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        SsButton().Checked({this, &NewProfilePage::OutboundTypeButton_Checked});
        TrojanButton().Checked({this, &NewProfilePage::OutboundTypeButton_Checked});
        HttpButton().Checked({this, &NewProfilePage::OutboundTypeButton_Checked});
    }

    void NewProfilePage::Page_Unloaded(IInspectable const & /* sender */, RoutedEventArgs const & /* e */)
    {
        SsButton().Checked(m_ssCheckedToken);
        TrojanButton().Checked(m_trojanCheckedToken);
        HttpButton().Checked(m_httpCheckedToken);
    }

    void NewProfilePage::NewProfileNameText_TextChanged(IInspectable const & /* sender */,
                                                        TextChangedEventArgs const & /* e */)
    {
        NewProfileNameText().Foreground({nullptr});
    }

}
