﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Microsoft.Azure.Devices;
using Microsoft.Azure.Devices.Common.Security;
using System.Net;

namespace DeviceExplorer
{
    public partial class SASTokenForm : Form
    {
        private RegistryManager registryManager;
        private string selectedDeviceId;
        private int devicesMaxCount;
        private string selectedDeviceKey;
        private string iotHubHostName;
        private IEnumerable<Device> iotHubDevices;
        private Dictionary<string, Device> devicesDictionary;

        public SASTokenForm(RegistryManager registryManager, int maxDevices, string iotHubHostName)
        {
            InitializeComponent();
            this.registryManager = registryManager;
            this.devicesMaxCount = maxDevices;
            this.iotHubHostName = iotHubHostName;
            initControls();
        }

        private async void initControls()
        {
            devicesDictionary = new Dictionary<string, Device>();

            try
            {
                this.iotHubDevices = await registryManager.GetDevicesAsync(devicesMaxCount);


                // Build the deviceID ComboBox and update the corresponding keys
                foreach (var device in iotHubDevices)
                {
                    devicesDictionary.Add(device.Id, device);
                }
                this.deviceIDComboBox.DataSource = devicesDictionary.Keys.OrderBy(c => c.ToLower()).ToList();
                this.selectedDeviceId = this.deviceIDComboBox.SelectedItem.ToString();
                updateKeysComboBox(this.selectedDeviceId);
            }
            catch (Exception ex)
            {
                using (new CenterDialog(this))
                {
                    MessageBox.Show(ex.Message);
                }
            }

        }

        private void updateKeysComboBox(string selectedDevice)
        {
            try
            {
                // Reconstruct the Device Keys combo box for that selected device
                List<string> deviceKeys = new List<string>();
                deviceKeys.Add(devicesDictionary[selectedDevice].Authentication.SymmetricKey.PrimaryKey);
                deviceKeys.Add(devicesDictionary[selectedDevice].Authentication.SymmetricKey.SecondaryKey);
                this.deviceKeyComboBox.DataSource = deviceKeys;
                selectedDeviceKey = this.deviceKeyComboBox.SelectedItem.ToString();
            }
            catch (Exception ex)
            {
                using (new CenterDialog(this))
                {
                    MessageBox.Show(ex.Message);
                }
            }
        }

        private void doneButton_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private string deviceConnectionStringWithSAS(string sas)
        {
            // Format of Device Connection String with SAS Token:
            // "HostName=<iothub_host_name>;CredentialType=SharedAccessSignature;DeviceId=<device_id>;SharedAccessSignature=SharedAccessSignature sr=<iot_host>/devices/<device_id>&sig=<token>&se=<expiry_time>";
            return String.Format("HostName={0};DeviceId={1};SharedAccessSignature={2}", iotHubHostName, selectedDeviceId, sas);
        }

        private void generateButton_Click(object sender, EventArgs e)
        {
            try
            {
                decimal ttlValue = numericUpDown1.Value;

                var sasBuilder = new SharedAccessSignatureBuilder()
                {
                    Key = this.selectedDeviceKey,
                    Target = String.Format("{0}/devices/{1}", iotHubHostName, WebUtility.UrlEncode(this.selectedDeviceId)),
                    TimeToLive = TimeSpan.FromDays(Convert.ToDouble(ttlValue))
                };
                
                sasRichTextBox.Text = deviceConnectionStringWithSAS(sasBuilder.ToSignature()) + "\r\n";

            }
            catch (Exception ex)
            {
                using (new CenterDialog(this))
                {
                    MessageBox.Show($"Unable to generate SAS. Verify connection strings.\n{ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }

        }

        private void deviceIDComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            this.selectedDeviceId = this.deviceIDComboBox.SelectedItem.ToString();
            updateKeysComboBox(this.selectedDeviceId);
        }

        private void deviceKeyComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            selectedDeviceKey = this.deviceKeyComboBox.SelectedItem.ToString();
        }
    }
}
