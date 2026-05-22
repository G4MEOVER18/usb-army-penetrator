using RJCP.IO.Ports;
using Microsoft.Win32;

namespace Agent
{
    public class DeviceNotFoundException : Exception
    {

    }

    internal class SerialComms
    {
        private readonly Executer executer = new();

        private List<string> GetMatchingSerialPorts(string vidToMatch, string pidToMatch)
        {
            vidToMatch = vidToMatch.ToUpperInvariant();
            pidToMatch = pidToMatch.ToUpperInvariant();

            const string usbDevicesPath = @"SYSTEM\CurrentControlSet\Enum\USB";
            using var usbKey = Registry.LocalMachine.OpenSubKey(usbDevicesPath);
            if (usbKey == null) return new List<string>();

            string searchPrefix = $"VID_{vidToMatch}&PID_{pidToMatch}";
            var ports = new List<string>();

            foreach (var deviceKeyName in usbKey.GetSubKeyNames())
            {
                if (!deviceKeyName.ToUpperInvariant().StartsWith(searchPrefix)) continue;

                using var deviceKey = usbKey.OpenSubKey(deviceKeyName);
                if (deviceKey == null) continue;

                foreach (var instanceKeyName in deviceKey.GetSubKeyNames())
                {
                    using var instanceKey = deviceKey.OpenSubKey(instanceKeyName);
                    using var deviceParams = instanceKey?.OpenSubKey("Device Parameters");
                    var portName = deviceParams?.GetValue("PortName") as string;
                    if (!string.IsNullOrWhiteSpace(portName) && !ports.Contains(portName))
                        ports.Add(portName);
                }
            }

            return ports;
        }

        public void Start(string vid, string pid, int baud = 115200)
        {
            var ports = GetMatchingSerialPorts(vid, pid);

            if (ports.Count == 0)
            {
                throw new DeviceNotFoundException();
            }

            foreach (var port in ports)
            {
                try
                {
                    ReadCommands(baud, port);
                    return;
                }
                catch (System.IO.IOException)
                {
                    // port is a ghost entry, try next
                }
            }

            throw new DeviceNotFoundException();
        }

        private void ReadCommands(int baud, object serialPortName)
        {
#if DEBUG
            Console.WriteLine($"Opening {serialPortName}");
#endif

            using (var cts = new CancellationTokenSource())
            using (var m_RxPort = new SerialPortStream(serialPortName.ToString())
            {
                BaudRate = 115200,
                DataBits = 8,
                Parity = Parity.None,
                StopBits = StopBits.One,
                ReadTimeout = -1,
                WriteTimeout = -1
            })
            {
                m_RxPort.Open();

                while (!cts.IsCancellationRequested)
                {
                    try
                    {
                        executer.ParseAndExecute(m_RxPort, cts);
                    }
                    catch (Exception)
                    {
                        cts.Cancel();
                        throw;
                    }
                }
            }

#if DEBUG
            Console.WriteLine("Serial port closed");
#endif
        }
    }
}
