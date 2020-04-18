using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// https://go.microsoft.com/fwlink/?LinkId=234238 上介绍了“空白页”项模板

namespace YtFlow.App.Pages
{
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class SettingPage : Page
    {
        public SettingPage ()
        {
            this.InitializeComponent();
        }

        private void ColorComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // Add "using Windows.UI;" for Color and Colors.
            string colorName = e.AddedItems[0].ToString();
            Color color;
            switch (colorName)
            {
                case "Yellow":
                    color = Colors.Yellow;
                    break;
                case "Green":
                    color = Colors.Green;
                    break;
                case "Blue":
                    color = Colors.Blue;
                    break;
                case "Red":
                    color = Colors.Red;
                    break;
            }
        }

        //private void OptionRadioButton_Checked(object sender, RoutedEventArgs e)
        //{

        //}

        //private void Option1RadioButton_Checked(object sender, RoutedEventArgs e)
        //{
            
        //}

        //private void Option2RadioButton_Checked(object sender, RoutedEventArgs e)
        //{

        //}

        private void RadioButtons_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // Add "using Windows.UI;" for Color and Colors.
            string colorName = e.AddedItems[0].ToString();
            Color color;
            switch (colorName)
            {              
                case "defalut":
                    color = default;
                    break;
                case "blue":
                    color = Colors.Blue;
                    break;
                case "pink":
                    color = Colors.Pink;
                    break;
            }
        }
    }
}
