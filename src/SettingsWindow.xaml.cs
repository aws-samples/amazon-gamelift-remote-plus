// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT-0

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace GameLiftRemotePlus
{
    public partial class SettingsWindow : Window
    {
        MainWindow main;
        bool init = false;

        public SettingsWindow(MainWindow _main)
        {
            main = _main;

            InitializeComponent();

            cidrCheckbox.IsChecked = main.useCustomCidr;
            customCidrTextBox.Text = main.customCidr;
            debugCheckbox.IsChecked = main.useRemoteDebugPort;
            debugPortTextBox.Text = main.remoteDebugPort.ToString();
            deleteCheckbox.IsChecked = main.showDeleteButtons;
            init = true;
            Validate();
        }

        private void customCidrTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            Validate();
        }

        private void debugPortTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            Validate();
        }

        private void Validate()
        {
            if (!init) return;

            if (cidrCheckbox.IsChecked != null) customCidrTextBox.IsEnabled = (bool)cidrCheckbox.IsChecked;
            if (debugCheckbox.IsChecked != null) debugPortTextBox.IsEnabled = (bool)debugCheckbox.IsChecked;

            var match = Regex.Match(customCidrTextBox.Text, @"^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\/([0-9]|[1-2][0-9]|3[0-2])$", RegexOptions.None);
            bool cidrValid = match.Success;

            UInt16 remoteDebugPort;
            bool debugPortValid = UInt16.TryParse(debugPortTextBox.Text, out remoteDebugPort);

            ok.IsEnabled = (!customCidrTextBox.IsEnabled || cidrValid) && (!debugPortTextBox.IsEnabled || debugPortValid);
        }

        private void ok_Click(object sender, RoutedEventArgs e)
        {
            main.useCustomCidr = (bool)cidrCheckbox.IsChecked;
            if (main.useCustomCidr) main.customCidr = customCidrTextBox.Text;
            main.useRemoteDebugPort = (bool)debugCheckbox.IsChecked;
            if (main.useRemoteDebugPort) UInt16.TryParse(debugPortTextBox.Text, out main.remoteDebugPort);
            main.showDeleteButtons = (bool)deleteCheckbox.IsChecked;
            main.SaveSettings();
            this.Close();
        }

        private void cancel_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }

        private void debugCheckbox_Click(object sender, RoutedEventArgs e)
        {
            Validate();
        }

        private void cidrCheckbox_Click(object sender, RoutedEventArgs e)
        {
            Validate();
        }

        private void deleteCheckbox_Click(object sender, RoutedEventArgs e)
        {
            // don't need to validate the input
        }
    }
}
