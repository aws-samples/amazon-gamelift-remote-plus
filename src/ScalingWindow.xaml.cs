// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT-0

using Amazon.GameLift;
using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace GameLiftRemotePlus
{
    public partial class ScalingWindow : Window
    {
        Amazon.RegionEndpoint currentRegion;
        string fleetId;
        MainWindow main;
        int realMin, realDes, realMax;
        bool init = false;

        public ScalingWindow(MainWindow _main, string _fleetId, Amazon.RegionEndpoint _currentRegion)
        {
            InitializeComponent();

            main = _main;
            currentRegion = _currentRegion;
            fleetId = _fleetId;

            this.Title = fleetId;

            var config = new AmazonGameLiftConfig();
            config.RegionEndpoint = currentRegion;
            using (AmazonGameLiftClient aglc = new AmazonGameLiftClient(config))
            {
                Amazon.GameLift.Model.DescribeFleetCapacityResponse ufcres = aglc.DescribeFleetCapacity(new Amazon.GameLift.Model.DescribeFleetCapacityRequest
                {
                    FleetIds = new List<string> { fleetId }
                });

                realMin = ufcres.FleetCapacity[0].InstanceCounts.MINIMUM;
                realDes = ufcres.FleetCapacity[0].InstanceCounts.DESIRED;
                realMax = ufcres.FleetCapacity[0].InstanceCounts.MAXIMUM;

                minVal.Text = realMin.ToString();
                desiredVal.Text = realDes.ToString();
                maxVal.Text = realMax.ToString();

                SetSliderBounds();
            }
            init = true;
        }

        private void ParseValues(out int min, out int desired, out int max)
        {
            if (Int32.TryParse(minVal.Text, out min))
            {
                minVal.Text = min.ToString();
            }
            else
            {
                minVal.Text = "";
                min = realMin;
            }
            if (Int32.TryParse(desiredVal.Text, out desired))
            {
                desiredVal.Text = desired.ToString();
            }
            else
            {
                desiredVal.Text = "";
                desired = realDes;
            }
            if (Int32.TryParse(maxVal.Text, out max))
            {
                maxVal.Text = max.ToString();
            }
            else
            {
                maxVal.Text = "";
                max = realMax;
            }
        }

        private void ok_Click(object sender, RoutedEventArgs e)
        {
            int min, desired, max;
            ParseValues(out min, out desired, out max);

            var config = new AmazonGameLiftConfig();
            config.RegionEndpoint = currentRegion;
            using (AmazonGameLiftClient aglc = new AmazonGameLiftClient(config))
            {
                try
                {
                    Amazon.GameLift.Model.UpdateFleetCapacityResponse ufcres = aglc.UpdateFleetCapacity(new Amazon.GameLift.Model.UpdateFleetCapacityRequest
                    {
                        MinSize = min,
                        DesiredInstances = desired,
                        MaxSize = max,
                        FleetId = fleetId
                    });
                }
                catch (Exception) {
                    MessageBox.Show(main, "You probably went over your service limits. Reduce your maximum instance count.", "Problem scaling fleet", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
            main.RefreshInstanceLabel();
            this.Close();
        }

        private void cancel_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }

        private bool IsTextAllowed(string text)
        {
            Regex regex = new Regex("^[0-9]+$");
            return regex.IsMatch(text);
        }

        private void ValidateValue(object sender, TextCompositionEventArgs e)
        {
            e.Handled = !IsTextAllowed(e.Text);
        }

        private void TextBoxPasting(object sender, DataObjectPastingEventArgs e)
        {
            if (e.DataObject.GetDataPresent(typeof(String)))
            {
                String text = (String)e.DataObject.GetData(typeof(String));
                if (!IsTextAllowed(text))
                {
                    e.CancelCommand();
                }
            }
            else
            {
                e.CancelCommand();
            }
        }

        private void minVal_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (init == false) return;
            init = false;
            int min, desired, max;
            ParseValues(out min, out desired, out max);

            realMin = min;

            if (min > desired)
                desiredVal.Text = min.ToString();
            if (min > max)
                maxVal.Text = min.ToString();
            if (desired > realDes)
                desiredVal.Text = Math.Max(min, realDes).ToString();
            if (max > realMax)
                maxVal.Text = Math.Max(Math.Max(min, realDes), realMax).ToString();
            SetSliderBounds();
            init = true;
        }

        private void desiredVal_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (init == false) return;
            init = false;
            int min, desired, max;
            ParseValues(out min, out desired, out max);

            realDes = desired;

            if (min > desired)
                minVal.Text = desired.ToString();
            if (desired > max)
                maxVal.Text = desired.ToString();
            if (min < realMin)
                minVal.Text = Math.Min(realMin, desired).ToString();
            if (max > realMax)
                maxVal.Text = Math.Max(desired, realMax).ToString();
            SetSliderBounds();
            init = true;
        }

        private void maxVal_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (init == false) return;
            init = false;
            int min, desired, max;
            ParseValues(out min, out desired, out max);

            realMax = max;

            if (desired > max)
                desiredVal.Text = max.ToString();
            if (min > max)
                minVal.Text = max.ToString();
            if (desired < realDes)
                desiredVal.Text = Math.Min(realDes, max).ToString();
            if (min < realMin)
                minVal.Text = Math.Min(Math.Min(realMin, realDes), max).ToString();
            SetSliderBounds();
            init = true;
        }

        private void slider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (init == false) return;
            init = false;
            int min, desired, max;
            ParseValues(out min, out desired, out max);

            desired = (int)slider.Value;
            realDes = desired;
            desiredVal.Text = desired.ToString();

            if (min > desired)
                minVal.Text = desired.ToString();
            if (desired > max)
                maxVal.Text = desired.ToString();
            if (min < realMin)
                minVal.Text = Math.Min(realMin, desired).ToString();
            if (max > realMax)
                maxVal.Text = Math.Max(desired, realMax).ToString();
            SetSliderBounds();
            init = true;

        }

        private void SetSliderBounds()
        {
            int min, desired, max;
            Int32.TryParse(minVal.Text, out min);
            Int32.TryParse(desiredVal.Text, out desired);
            Int32.TryParse(maxVal.Text, out max);

            slider.Minimum = Math.Max(0, min - 5);
            slider.Value = desired;
            slider.Maximum = Math.Max(max, min + 20);
        }
    }
}
