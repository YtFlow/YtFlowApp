#include "pch.h"
#include "AboutPage.h"
#if __has_include("AboutPage.g.cpp")
#include "AboutPage.g.cpp"
#endif

#include "CoreFfi.h"
#include "winrt/Windows.System.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;

namespace winrt::YtFlowApp::implementation
{
    void AboutPage::Page_Loaded(IInspectable const &, RoutedEventArgs const &)
    {
        CoreVersionText().Text(to_hstring(GetYtFlowCoreVersion()));
        auto const pkgVer = Windows::ApplicationModel::Package::Current().Id().Version();
        auto pkgVerStr = to_hstring(pkgVer.Major) + L"." + to_hstring(pkgVer.Minor) + L"." + to_hstring(pkgVer.Build) +
                         L"." + to_hstring(pkgVer.Revision);
        PackageVersionText().Text(std::move(pkgVerStr));
    }

    fire_and_forget AboutPage::LicenseButton_Click(IInspectable const &, RoutedEventArgs const &)
    {
        auto const lifetime = get_strong();
        // .txt files do not work.
        auto const file =
            co_await Windows::Storage::StorageFile::GetFileFromApplicationUriAsync(Uri{L"ms-appx:///third_party.md"});
        co_await resume_foreground(lifetime->Dispatcher());
        Windows::System::LauncherOptions const options;
        options.DisplayApplicationPicker(true);
        co_await Windows::System::Launcher::LaunchFileAsync(file, options);
    }

}
