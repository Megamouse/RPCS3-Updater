using System;
using System.IO;
using System.Net;
using Newtonsoft.Json;

class RPCS3Updater
{
    public static void Main(string[] args)
    {
        new Downloader();
    }
}

public class Downloader
{
    WebClient dw     = new WebClient();
    String    api    = "https://update.rpcs3.net/?c=XXXXXXXX";
    String    latest;
    
    public Downloader()
    {
        latest = pingBani(api);
        dw.DownloadFile(new Uri(latest), @"balls.7z");
    }

    public string pingBani(String apiURL)
    {
        String s = "";

        HttpWebRequest apiPing = (HttpWebRequest) WebRequest.Create(apiURL);
        JsonTextReader json;

        using (WebResponse response = apiPing.GetResponse())
        {
            using (StreamReader the = new StreamReader(response.GetResponseStream()))
            {
                json = new JsonTextReader(the);

                while (json.Read())
                {
                    if (json.Value != null)
                    {
                        if (json.Value.ToString().Contains("appveyor"))
                        {
                            s = json.Value.ToString();
                            Console.WriteLine("The response is: " + s);
                        }
                    }
                }
            }
        }

        return s;
    }
}