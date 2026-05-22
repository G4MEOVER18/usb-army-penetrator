
using Task = System.Threading.Tasks.Task;
using Agent;
using System.Diagnostics;
using static Agent.Executer;
using System;
#if DEBUG
using Agent.LocalTesting;
#endif

class Program
{
    public static async Task Main(string[] args)
    {
        // Set the current working directory if we were given one on the cli
        args.Where(x => x.StartsWith("cwd=")).Select(x => x.Remove(0, 4)).ToList().ForEach(x => Environment.CurrentDirectory = Environment.ExpandEnvironmentVariables(x));

        using (Mutex mutex = new Mutex(true, "USBArmyKnifeAgent_v1", out bool createdNew))
        {
            if (!createdNew)
            {
#if DEBUG
                Console.WriteLine("Already running, quitting");
#endif
                return;
            }

#if DEBUG
            if (args.Any(x => x == "tcp"))
            {
                await TcpServer.Start(7000);
            }
            else if (args.Any(x => x == "web"))
            {
                WebServer.Start(7000, Environment.CurrentDirectory);
            }
            else
            {
#endif
                var vid = args.Where(x => x.StartsWith("vid=")).Single();
                var pid = args.Where(x => x.StartsWith("pid=")).Single();
                vid = vid.Split("=")[1].ToUpperInvariant();
                pid = pid.Split("=")[1].ToUpperInvariant();

                SerialComms serial = new();
                while (true)
                {
                    try
                    {
                        serial.Start(vid, pid);
                    }
                    catch (DeviceNotFoundException)
                    {
                        await Task.Delay(TimeSpan.FromSeconds(1));
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex);
                        await Task.Delay(TimeSpan.FromSeconds(1));
                    }
                }
#if DEBUG
            }
#endif
        }
    }
}
