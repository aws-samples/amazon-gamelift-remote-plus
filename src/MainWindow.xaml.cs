// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT-0

using Amazon.GameLift;
using Amazon.GameLift.Model;
using System;
using System.Windows;
using System.Windows.Controls;
using System.Diagnostics;
using System.IO;
using System.Collections.Generic;
using Microsoft.Win32;
using System.Net;
using System.Text.RegularExpressions;
using System.Windows.Input;
using System.Threading.Tasks;
using System.Reflection;
using CLR;
using System.Windows.Data;

namespace GameLiftRemotePlus
{
    [ValueConversion(typeof(bool), typeof(bool))]
    public class InverseBooleanConverter : IValueConverter
    {
        #region IValueConverter Members

        public object Convert(object value, Type targetType, object parameter,
            System.Globalization.CultureInfo culture)
        {
            if (targetType != typeof(bool))
                throw new InvalidOperationException("The target must be a boolean");

            return !(bool)value;
        }

        public object ConvertBack(object value, Type targetType, object parameter,
            System.Globalization.CultureInfo culture)
        {
            throw new NotSupportedException();
        }

        #endregion
    }

    public partial class MainWindow : Window
    {
        Amazon.RegionEndpoint currentRegion = Amazon.RegionEndpoint.USEast1;

        private struct FleetPermission
        {
            public string FleetId;
            public List<IpPermission> PermissionList;
        };

        private List<FleetPermission> openPorts;

        public bool useCustomCidr = false;
        public string customCidr = "0.0.0.0/0";
        public bool useRemoteDebugPort = false;
        public UInt16 remoteDebugPort = 2345;
        public bool showDeleteButtons = false;

        public MainWindow()
        {
            openPorts = new List<FleetPermission>();
            LoadSettings();
            InitializeComponent();
            SelectRegionFromConfig();
            Task.Run(() => RefreshRegionsList());
            StartDispatcherTimer();
        }

        bool isTimerUpdate = false;

        private System.Windows.Threading.DispatcherTimer dispatcherTimer;

        private void StartDispatcherTimer()
        {
            dispatcherTimer = new System.Windows.Threading.DispatcherTimer();
            dispatcherTimer.Tick += new EventHandler(dispatcherTimer_Tick);
            dispatcherTimer.Interval = new TimeSpan(0, 0, 30);
            dispatcherTimer.Start();
        }

        private void dispatcherTimer_Tick(object sender, EventArgs e)
        {
            isTimerUpdate = true;
            RefreshRegionsList();
            isTimerUpdate = false;

            CommandManager.InvalidateRequerySuggested();
        }

        private void LoadSettings()
        {
            try
            {
                string path = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
                string settingFile = Path.Combine(path, "setting.txt");
                string[] lines = File.ReadAllLines(settingFile);
                foreach (string line in lines)
                {
                    string _line = line.ToLower();
                    Regex.Replace(_line, @"\s+", "");
                    var tokens = _line.Split(':');
                    if (tokens[0] == "usecustomcidr")
                    {
                        if (tokens[1] == "false") useCustomCidr = false;
                        if (tokens[1] == "true") useCustomCidr = true;
                    }
                    if (tokens[0]=="customcidr")
                    {
                        if (Regex.Match(tokens[1], @"^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])/([0-9]|[12][0-9]|3[0-2])$").Success)
                            customCidr = tokens[1];
                    }
                    if (tokens[0] == "usedebugport")
                    {
                        if (tokens[1] == "false") useRemoteDebugPort = false;
                        if (tokens[1] == "true") useRemoteDebugPort = true;
                    }
                    if (tokens[0] == "debugport")
                    {
                        UInt16.TryParse(tokens[1], out remoteDebugPort);
                    }
                    if (tokens[0] == "showdeletebuttons")
                    {
                        if (tokens[1] == "false") showDeleteButtons = false;
                        if (tokens[1] == "true") showDeleteButtons = true;
                    }
                }
            }
            catch(Exception)
            {
                // no settings file found, not fatal
            }
        }

        public void SaveSettings()
        {
            string[] lines = new string[6];
            lines[0] = "#GameLiftRemotePlus configuration. Keyword colon value format";
            lines[1] = "usecustomcidr:" + (useCustomCidr ? "true" : "false");
            lines[2] = "customcidr:" + customCidr;
            lines[3] = "usedebugport:" + (useRemoteDebugPort ? "true" : "false");
            lines[4] = "debugport:" + remoteDebugPort.ToString();
            lines[5] = "showdeletebuttons:" + (showDeleteButtons ? "true" : "false");
            string path = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            string settingFile = Path.Combine(path, "setting.txt");
            File.WriteAllLines(settingFile, lines);
            UpdateFleetControlState();
            UpdateAliasControlState();
            UpdateBuildControlState();
        }

        private void SelectRegionFromConfig()
        {
            string configFile = Path.Combine(System.Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), @".aws\config");
            var chain = new Amazon.Runtime.CredentialManagement.SharedCredentialsFile(configFile);
            Amazon.Runtime.CredentialManagement.CredentialProfile profile;
            if (chain.TryGetProfile("default", out profile))
            {
                currentRegion = profile.Region;
            }
        }

        private void RefreshRegionsList()
        {
            // Run on dispatcher thread
            if (!Application.Current.Dispatcher.CheckAccess())
            {
                Application.Current.Dispatcher.Invoke(new System.Action(() => RefreshRegionsList()));
                return;
            }

            select_region.Items.Clear();
            foreach(Amazon.RegionEndpoint region in Amazon.RegionEndpoint.EnumerableAllRegions)
            {
                var item = new ComboBoxItem
                {
                    Content = region.DisplayName,
                    Tag = region.SystemName
                };
                select_region.Items.Add(item);
                if (region == currentRegion)
                    select_region.SelectedItem = item;
            }

            Task.Run(() => RefreshFleetsList());
            Task.Run(() => RefreshAliasesList());
            Task.Run(() => RefreshBuildsList());
        }

        private void RefreshFleetsList()
        {
            // Run on dispatcher thread
            if (!Application.Current.Dispatcher.CheckAccess())
            {
                Application.Current.Dispatcher.Invoke(new System.Action(() => RefreshFleetsList()));
                return;
            }

            ResetFleetControlState();
            string selected = null;
            if ((ListBoxItem)select_fleet.SelectedItem != null)
                selected = ((ListBoxItem)select_fleet.SelectedItem).Content.ToString().Substring(0, 42);

            var config = new AmazonGameLiftConfig();
            config.RegionEndpoint = currentRegion;
            using (AmazonGameLiftClient aglc = new AmazonGameLiftClient(config))
            {
                try
                {
                    string token = null;
                    select_fleet.Items.Clear();
                    scale_fleet.IsEnabled = false;
                    Amazon.GameLift.Model.DescribeFleetAttributesResponse dfares;
                    do
                    {
                        dfares = aglc.DescribeFleetAttributes(new Amazon.GameLift.Model.DescribeFleetAttributesRequest
                        {
                            Limit = 20,
                            NextToken = token
                        });
                        foreach (FleetAttributes attr in dfares.FleetAttributes)
                        {
                            string os = attr.OperatingSystem == "WINDOWS_2012" ? "WN" : "LX";
                            var item = new ListBoxItem{
                                Content = attr.FleetId + " (" + attr.Name + ")",
                                Tag = os
                            };
                            if (attr.Status == "DELETING" || attr.Status == "TERMINATED")
                            {
                                item.IsEnabled = false;
                            }
                            if (attr.Status != "ACTIVE")
                            {
                                item.Content += " - " + attr.Status;
                            }
                            select_fleet.Items.Add(item);
                            if (attr.FleetId == selected) item.IsSelected = true;
                        }
                        token = dfares.NextToken;
                    }
                    while (token != null);
                }
                catch (Exception) { }
            }
            UpdateFleetControlState();
        }

        private void RefreshInstancesList()
        {
            // Run on dispatcher thread
            if (!Application.Current.Dispatcher.CheckAccess())
            {
                Application.Current.Dispatcher.Invoke(new System.Action(() => RefreshInstancesList()));
                return;
            }

            string selected = null;
            if ((ListBoxItem)select_instance.SelectedItem != null)
                selected = ((ListBoxItem)select_instance.SelectedItem).Content.ToString().Substring(0, 19);

            select_instance.Items.Clear();

            var fleetItem = (ListBoxItem)select_fleet.SelectedItem;
            if (fleetItem == null) return;
            string fleetId = fleetItem.Content.ToString().Substring(0, 42);

            var config = new AmazonGameLiftConfig();
            config.RegionEndpoint = currentRegion;
            using (AmazonGameLiftClient aglc = new AmazonGameLiftClient(config))
            {
                try
                {
                    string token = null;
                    Amazon.GameLift.Model.DescribeInstancesResponse dires;
                    do
                    {
                        dires = aglc.DescribeInstances(new Amazon.GameLift.Model.DescribeInstancesRequest{
                            Limit = 20,
                            NextToken = token,
                            FleetId = fleetId
                        });
                        foreach (Instance inst in dires.Instances)
                        {
                            var item = new ListBoxItem();
                            string ip = String.IsNullOrEmpty(inst.IpAddress) ? "" : " (" + inst.IpAddress + ")";
                            item.Content = inst.InstanceId + ip;
                            if (inst.Status != "ACTIVE")
                            {
                                item.IsEnabled = false;
                                item.Content += " - " + inst.Status;
                            }
                            select_instance.Items.Add(item);
                            if (inst.InstanceId == selected) item.IsSelected = true;
                        }
                        token = dires.NextToken;
                    }
                    while (token != null);
                }
                catch (Exception) { }
            }
        }

        private void RefreshAliasesList()
        {
            // Run on dispatcher thread
            if (!Application.Current.Dispatcher.CheckAccess())
            {
                Application.Current.Dispatcher.Invoke(new System.Action(() => RefreshAliasesList()));
                return;
            }

            ResetAliasControlState();
            string selected = null;
            if ((ComboBoxItem)select_alias.SelectedItem != null)
                selected = ((ComboBoxItem)select_alias.SelectedItem).Content.ToString().Substring(0, 42);

            var config = new AmazonGameLiftConfig();
            config.RegionEndpoint = currentRegion;
            using (AmazonGameLiftClient aglc = new AmazonGameLiftClient(config))
            {
                try
                {
                    string token = null;
                    select_alias.Items.Clear();
                    Amazon.GameLift.Model.ListAliasesResponse lares;
                    do
                    {
                        lares = aglc.ListAliases(new Amazon.GameLift.Model.ListAliasesRequest
                        {
                            Limit = 20,
                            NextToken = token
                        });
                        foreach (Amazon.GameLift.Model.Alias alias in lares.Aliases)
                        {
                            string message = "";
                            bool found = false;
                            if (alias.RoutingStrategy.Type == "SIMPLE")
                            {
                                foreach (ListBoxItem fleetItem in select_fleet.Items)
                                {
                                    if (fleetItem.Content.ToString().Substring(0, 42) == alias.RoutingStrategy.FleetId)
                                    {
                                        found = true;
                                        break;
                                    }
                                }
                                message = alias.RoutingStrategy.FleetId + (found ? "" : " NOT FOUND");
                            }
                            else
                                message = alias.RoutingStrategy.Type;
                            
                            var item = new ComboBoxItem
                            {
                                Content = alias.AliasId + " (" + alias.Name + ") -> " + message,
                                Tag = (found ? alias.RoutingStrategy.FleetId : "")
                            };
                            select_alias.Items.Add(item);
                            if (alias.AliasId == selected) item.IsSelected = true;

                        }
                        token = lares.NextToken;
                    }
                    while (token != null);
                }
                catch (Exception) { }
            }
            UpdateAliasControlState();
        }

        private void RefreshBuildsList()
        {
            // Run on dispatcher thread
            if (!Application.Current.Dispatcher.CheckAccess())
            {
                Application.Current.Dispatcher.Invoke(new System.Action(() => RefreshBuildsList()));
                return;
            }

            ResetBuildControlState();
            string selected = null;
            if ((ComboBoxItem)select_build.SelectedItem != null)
                selected = ((ComboBoxItem)select_build.SelectedItem).Content.ToString().Substring(0, 42);

            var config = new AmazonGameLiftConfig();
            config.RegionEndpoint = currentRegion;
            using (AmazonGameLiftClient aglc = new AmazonGameLiftClient(config))
            {
                try
                {
                    string token = null;
                    select_build.Items.Clear();
                    Amazon.GameLift.Model.ListBuildsResponse lbres;
                    do
                    {
                        lbres = aglc.ListBuilds(new Amazon.GameLift.Model.ListBuildsRequest
                        {
                            Limit = 20,
                            NextToken = token
                        });
                        foreach (Amazon.GameLift.Model.Build build in lbres.Builds)
                        {
                            var item = new ComboBoxItem
                            {
                                Content = build.BuildId + " (" + build.Name + ")"
                            };
                            if (build.Status == "INITIALIZED")
                            {
                                item.IsEnabled = false;
                            }
                            select_build.Items.Add(item);
                            if (build.BuildId == selected) item.IsSelected = true;

                        }
                        token = lbres.NextToken;
                    }
                    while (token != null);
                }
                catch (Exception) { }
            }
            UpdateBuildControlState();
        }

        private void scale_fleet_Click(object sender, RoutedEventArgs e)
        {
            var fleetItem = (ListBoxItem)select_fleet.SelectedItem;
            if (fleetItem == null) return;
            string fleetId = fleetItem.Content.ToString().Substring(0, 42);

            new ScalingWindow(this, fleetId, currentRegion).Show();
        }

        private void view_fleet_Click(object sender, RoutedEventArgs e)
        {
            var fleetItem = (ListBoxItem)select_fleet.SelectedItem;
            if (fleetItem == null) return;
            string fleetId = fleetItem.Content.ToString().Substring(0, 42);
            System.Diagnostics.Process.Start("https://console.aws.amazon.com/gamelift/home?region=" + currentRegion.SystemName + "#/r/fleets/" + fleetId);
        }

        private void delete_fleet_Click(object sender, RoutedEventArgs e)
        {
            var fleetItem = (ListBoxItem)select_fleet.SelectedItem;
            if (fleetItem == null) return;
            string fleetId = fleetItem.Content.ToString().Substring(0, 42);
            Task.Run(() => DeleteFleet(fleetId));
        }

        private void DeleteFleet(string fleetId)
        {
            var config = new AmazonGameLiftConfig();
            config.RegionEndpoint = currentRegion;
            using (AmazonGameLiftClient aglc = new AmazonGameLiftClient(config))
            {
                try
                {
                    Amazon.GameLift.Model.UpdateFleetCapacityResponse ufcres = aglc.UpdateFleetCapacity(new Amazon.GameLift.Model.UpdateFleetCapacityRequest
                    {
                        MinSize = 0,
                        DesiredInstances = 0,
                        MaxSize = 0,
                        FleetId = fleetId
                    });

                    Amazon.GameLift.Model.DeleteFleetResponse dfres = aglc.DeleteFleet(new Amazon.GameLift.Model.DeleteFleetRequest
                    {
                        FleetId = fleetId
                    });
                }
                catch (Exception) { }
            }
            Task.Run(() => RefreshFleetsList());
        }

        private void view_build_Click(object sender, RoutedEventArgs e)
        {
            var buildItem = (ComboBoxItem)select_build.SelectedItem;
            if (buildItem == null) return;
            string buildId = buildItem.Content.ToString().Substring(0, 42);

            System.Diagnostics.Process.Start("https://console.aws.amazon.com/gamelift/home?region=" + currentRegion.SystemName + "#/r/builds/" + buildId);
        }

        private void view_alias_Click(object sender, RoutedEventArgs e)
        {
            var aliasItem = (ComboBoxItem)select_alias.SelectedItem;
            if (aliasItem == null) return;
            string aliasId = aliasItem.Content.ToString().Substring(0, 42);

            System.Diagnostics.Process.Start("https://console.aws.amazon.com/gamelift/home?region=" + currentRegion.SystemName + "#/r/aliases/" + aliasId);
        }

        private void ResetFleetControlState()
        {
            scale_fleet.IsEnabled = false;
            view_fleet.IsEnabled = false;
            delete_fleet.IsEnabled = false;
        }

        private void UpdateFleetControlState()
        {
            ResetFleetControlState();
            delete_fleet.ToolTip = showDeleteButtons? "Select a fleet" : "Enable in settings";
            if ((ListBoxItem)select_fleet.SelectedItem == null)
            {
                return;
            }
            string content = ((ListBoxItem)select_fleet.SelectedItem).Content.ToString();
            string fleetState = content.Substring(Math.Max(0, content.Length - 5));
            scale_fleet.IsEnabled = (fleetState != "ERROR") && (fleetState != "ETING") && (fleetState != "NATED"); // !error&&!deleting&&!terminated
            view_fleet.IsEnabled = true;
            delete_fleet.IsEnabled = (fleetState[4] == ')') || (fleetState == "ERROR") ? showDeleteButtons : false; // active||error
        }

        private void ResetAliasControlState()
        {
            view_alias.IsEnabled = false;
            delete_alias.IsEnabled = false;
        }

        private void UpdateAliasControlState()
        {
            ResetAliasControlState();
            delete_alias.ToolTip = showDeleteButtons ? "Select an alias" : "Enable in settings";
            if ((ComboBoxItem)select_alias.SelectedItem == null)
            {
                return;
            }
            view_alias.IsEnabled = true;
            delete_alias.IsEnabled = showDeleteButtons;
        }

        private void ResetBuildControlState()
        {
            create_fleet.IsEnabled = false;
            view_build.IsEnabled = false;
            delete_build.IsEnabled = false;
        }

        private void UpdateBuildControlState()
        {
            ResetBuildControlState();
            delete_build.ToolTip = showDeleteButtons? "Select a build" : "Enable in settings";
            if ((ComboBoxItem)select_build.SelectedItem == null)
            {
                return;
            }
            create_fleet.IsEnabled = true;
            view_build.IsEnabled = true;
            delete_build.IsEnabled = showDeleteButtons;
        }

        private void select_fleet_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            UpdateFleetControlState();
            Task.Run(() => RefreshInstancesList());
            Task.Run(() => RefreshInstanceLabel());
        }

        public void RefreshInstanceLabel()
        {
            // Run on dispatcher thread
            if (!Application.Current.Dispatcher.CheckAccess())
            {
                Application.Current.Dispatcher.Invoke(new System.Action(() => RefreshInstanceLabel()));
                return;
            }

            instancesLabel.Content = "Instances";
            if ((ListBoxItem)select_fleet.SelectedItem == null) return;
            string fleetId = ((ListBoxItem)select_fleet.SelectedItem).Content.ToString().Substring(0, 42);

            var config = new AmazonGameLiftConfig();
            config.RegionEndpoint = currentRegion;
            using (AmazonGameLiftClient aglc = new AmazonGameLiftClient(config))
            {
                try
                {
                    Amazon.GameLift.Model.DescribeFleetCapacityResponse ufcres = aglc.DescribeFleetCapacity(new Amazon.GameLift.Model.DescribeFleetCapacityRequest
                    {
                        FleetIds = new List<string> { fleetId }
                    });

                    int min = ufcres.FleetCapacity[0].InstanceCounts.MINIMUM;
                    int desired = ufcres.FleetCapacity[0].InstanceCounts.DESIRED;
                    int active = ufcres.FleetCapacity[0].InstanceCounts.ACTIVE;
                    int idle = ufcres.FleetCapacity[0].InstanceCounts.IDLE;
                    int max = ufcres.FleetCapacity[0].InstanceCounts.MAXIMUM;

                    instancesLabel.Content = "Instances       Min " + min + "   Desired " + desired + "   [ Active " + active + "   Idle " + idle + " ]   Max " + max;
                }
                catch (Exception) { }
            }

        }

        private void select_instance_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            connect_instance.IsEnabled = select_instance.SelectedItems.Count != 0;
        }

        private void RunCmd(string exe, string args, bool waitForExit = true)
        {
            Process cmd = new Process();
            cmd.StartInfo.FileName = exe;
            cmd.StartInfo.Arguments = args;
            cmd.StartInfo.RedirectStandardInput = true;
            cmd.StartInfo.RedirectStandardOutput = true;
            cmd.StartInfo.RedirectStandardError = true;
            cmd.StartInfo.CreateNoWindow = true;
            cmd.StartInfo.UseShellExecute = false;
            cmd.Start();

            if (waitForExit)
            {
                cmd.WaitForExit();

                if (cmd.ExitCode != 0)
                {
                    MessageBox.Show(cmd.StandardError.ReadToEnd());
                }
            }
            return;
        }

        private void FixDefaultRdpFile()
        {
            // Fix RDP file. If this is not done, then the user may be prompted for credentials even though the credentials are stored.
            string defaultRdpFile = Path.Combine(System.Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "Default.rdp");
            if (File.Exists(defaultRdpFile))
            {
                string[] lines = File.ReadAllLines(defaultRdpFile);
                for (int idx = 0; idx < lines.Length; idx++ )
                {
                    if (lines[idx] == "prompt for credentials:i:1")
                    {
                        lines[idx] = "prompt for credentials:i:0";
                        // unhide the file if it is hidden so it can be written
                        FileAttributes attributes = File.GetAttributes(defaultRdpFile);
                        File.SetAttributes(defaultRdpFile, attributes & ~FileAttributes.Hidden);
                        // write the modified file
                        File.WriteAllLines(defaultRdpFile, lines);
                        // return the file attributes to their original state
                        File.SetAttributes(defaultRdpFile, attributes);
                        return;
                    }
                }
            }
        }

        private void connect_instance_Click(object sender, RoutedEventArgs e)
        {
            connect_instance.IsEnabled = false;

            string fleetId = ((ListBoxItem)select_fleet.SelectedItem).Content.ToString().Substring(0, 42);
            string instanceId = ((ListBoxItem)select_instance.SelectedItem).Content.ToString().Substring(0, 19);
            string os = ((ListBoxItem)select_fleet.SelectedItem).Tag.ToString().Substring(0, 2); // WN or LX
            Amazon.GameLift.Model.GetInstanceAccessResponse giares;

            var config = new AmazonGameLiftConfig();
            config.RegionEndpoint = currentRegion;
            using (AmazonGameLiftClient aglc = new AmazonGameLiftClient(config))
            {
                try
                {
                    giares = aglc.GetInstanceAccess(new GetInstanceAccessRequest
                    {
                        FleetId = fleetId,
                        InstanceId = instanceId
                    }); 
                }
                catch (Exception)
                {
                    return;
                }
            }

            OpenRemoteAccessPort(fleetId, os);

            if (os == "WN")
            {
                FixDefaultRdpFile();

                // Invoke remote desktop
                RunCmd("cmdkey", String.Format(@"/generic:TERMSRV/{0} /user:{1} /pass:{2}", giares.InstanceAccess.IpAddress, giares.InstanceAccess.Credentials.UserName, giares.InstanceAccess.Credentials.Secret));
                RunCmd("mstsc", String.Format(@"/v:{0}", giares.InstanceAccess.IpAddress));
            }
            else
            {
                string path = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
                string secret = giares.InstanceAccess.Credentials.Secret;
                File.WriteAllText(Path.Combine(path, giares.InstanceAccess.InstanceId + ".pem"), secret);
                CLR.KeyConvert.Convert(Path.Combine(path, giares.InstanceAccess.InstanceId + ".pem"), Path.Combine(path, giares.InstanceAccess.InstanceId + ".ppk"));

                path = Path.Combine(path, giares.InstanceAccess.InstanceId + ".ppk");

                // Save a PuTTY session so user can access instances again directly with PuTTY
                RegistryKey key = Registry.CurrentUser.OpenSubKey("Software", true);
                key = key.CreateSubKey("SimonTatham");
                key = key.CreateSubKey("PuTTY");
                key = key.CreateSubKey("Sessions");
                RegistryKey session = key.CreateSubKey(giares.InstanceAccess.InstanceId);
                session.SetValue("HostName", giares.InstanceAccess.IpAddress);
                session.SetValue("UserName", giares.InstanceAccess.Credentials.UserName);
                session.SetValue("PublicKeyFile", path);

                // Invoke PuTTY
                GetPutty();
                RunCmd("putty", String.Format(@"-load {0}", giares.InstanceAccess.InstanceId, ""), false); // don't wait
            }

            connect_instance.IsEnabled = true;
        }

        private void GetPutty()
        {
            string path = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            path = Path.Combine(path, "putty.exe");

            if (!File.Exists(path))
            {
                ServicePointManager.Expect100Continue = true;
                ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;
                var webClient = new System.Net.WebClient();
                string os_bits = Environment.Is64BitOperatingSystem ? "64" : "32";
                webClient.DownloadFile(@"https://the.earth.li/~sgtatham/putty/latest/w" + os_bits + "/putty.exe", path);
            }
        }

        private void select_region_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var item = (ListBoxItem)select_region.SelectedItem;
            if (item == null) return;
            currentRegion = Amazon.RegionEndpoint.GetBySystemName(item.Tag.ToString());
            Task.Run(() => RefreshFleetsList());
            Task.Run(() => RefreshAliasesList());
            Task.Run(() => RefreshBuildsList());
        }

        private void select_alias_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var item = (ListBoxItem)select_alias.SelectedItem;
            ResetAliasControlState();
            if (item == null) return;
            string fleetId = item.Tag.ToString();

            if (!isTimerUpdate) // don't reselect fleet pointed to by alias if alias not selected by a human
            {
                foreach (ListBoxItem fleetItem in select_fleet.Items)
                {
                    if (fleetItem.Content.ToString().Substring(0, 42) == fleetId)
                    {
                        fleetItem.IsSelected = true;
                    }
                }
            }
            UpdateAliasControlState();
        }

        private void select_build_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            UpdateBuildControlState();
        }

        private void create_fleet_Click(object sender, RoutedEventArgs e)
        {
            string buildId = ((ListBoxItem)select_build.SelectedItem).Content.ToString().Substring(0, 42);
            System.Diagnostics.Process.Start("https://console.aws.amazon.com/gamelift/home?region=" + currentRegion.SystemName + "#/r/fleets/create?buildId=" + buildId);
        }

        private void ModifyPortConfiguration(string fleetId, List<IpPermission> permissionList, bool open)
        {
            UpdateFleetPortSettingsRequest modifyPortRequest;

            if (open)
            {
                modifyPortRequest = new UpdateFleetPortSettingsRequest
                {
                    FleetId = fleetId,
                    InboundPermissionAuthorizations = permissionList
                };
            }
            else
            {
                modifyPortRequest = new UpdateFleetPortSettingsRequest
                {
                    FleetId = fleetId,
                    InboundPermissionRevocations = permissionList
                };
            }

            var config = new AmazonGameLiftConfig();
            config.RegionEndpoint = currentRegion;
            using (AmazonGameLiftClient aglc = new AmazonGameLiftClient(config))
            {
                try
                {
                    UpdateFleetPortSettingsResponse ufpsres = aglc.UpdateFleetPortSettings(modifyPortRequest);
                }
                catch (InvalidRequestException)
                {
                    // non fatal, this happens if the port is already opened
                }
            }
        }

        private void OpenPortConfiguration(string fleetId, int port, bool tcp)
        {
            string ipAddress, cidr;
            // Use custom CIDR if set.
            if (useCustomCidr)
                cidr = customCidr;
            else
            {
                string url = "http://bot.whatismyipaddress.com/";
                using (WebClient client = new WebClient())
                {
                    ipAddress = client.DownloadString(url);
                }
                if (string.IsNullOrEmpty(ipAddress) || !Regex.Match(ipAddress, @"^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$").Success)
                    cidr = "0.0.0.0/0";
                else
                    cidr = ipAddress + "/32";
            }
            // Open the remote connection port
            List<IpPermission> permissionList = new List<IpPermission>();
            permissionList.Add(new IpPermission
            {
                FromPort = port,
                ToPort = port,
                IpRange = cidr, 
                Protocol = tcp ? IpProtocol.TCP : IpProtocol.UDP
            });

            ModifyPortConfiguration(fleetId, permissionList, true);

            openPorts.Add(new FleetPermission
            {
                FleetId = fleetId,
                PermissionList = permissionList,
            });
        }

        private void OpenRemoteAccessPort(string fleetId, string os)
        {
            OpenPortConfiguration(fleetId, os == "LX" ? 22 : 3389, true);
            if (useRemoteDebugPort)
                OpenPortConfiguration(fleetId, remoteDebugPort, true);
        }

        private void ClosePorts()
        {
            foreach (FleetPermission permission in openPorts)
            {
                ModifyPortConfiguration(permission.FleetId, permission.PermissionList, false);
            }
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (openPorts.Count != 0)
            {
                var result = MessageBox.Show(this,
                    "Ports were opened on your fleets to allow remote or debug connections. Close remote/debug ports to secure the fleets?",
                    "Fleet security", MessageBoxButton.YesNo, MessageBoxImage.Warning);
                if (result == MessageBoxResult.Yes)
                {
                    ClosePorts();
                }
            }
        }

        private void settings_Click(object sender, RoutedEventArgs e)
        {
            new SettingsWindow(this).Show();
        }

        private void delete_build_Click(object sender, RoutedEventArgs e)
        {
            var buildItem = (ComboBoxItem)select_build.SelectedItem;
            if (buildItem == null) return;
            string buildId = buildItem.Content.ToString().Substring(0, 42);

            var config = new AmazonGameLiftConfig();
            config.RegionEndpoint = currentRegion;
            using (AmazonGameLiftClient aglc = new AmazonGameLiftClient(config))
            {
                try
                {
                    Amazon.GameLift.Model.DeleteBuildResponse dfres = aglc.DeleteBuild(new Amazon.GameLift.Model.DeleteBuildRequest
                    {
                        BuildId = buildId
                    });
                }
                catch (Exception) { }
            }
            Task.Run(() => RefreshBuildsList());
        }

        private void delete_alias_Click(object sender, RoutedEventArgs e)
        {
            var aliasItem = (ComboBoxItem)select_alias.SelectedItem;
            if (aliasItem == null) return;
            string aliasId = aliasItem.Content.ToString().Substring(0, 42);

            var config = new AmazonGameLiftConfig();
            config.RegionEndpoint = currentRegion;
            using (AmazonGameLiftClient aglc = new AmazonGameLiftClient(config))
            {
                try
                {
                    Amazon.GameLift.Model.DeleteAliasResponse dfres = aglc.DeleteAlias(new Amazon.GameLift.Model.DeleteAliasRequest
                    {
                        AliasId = aliasId
                    });
                }
                catch (Exception) { }
            }
            Task.Run(() => RefreshAliasesList());
        }
    }
}
