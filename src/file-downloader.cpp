#include <network-monitor/file-downloader.h>

#include <cpr/cpr.h>

#include <filesystem>
#include <string>
#include <iostream>
#include <fstream>

bool NetworkMonitor::DownloadFile(
    const std::string& fileUrl,
    const std::filesystem::path& destination,
    const std::filesystem::path& caCertFile
)
{
    // Add certificate to verify server against and get the json file from the url
    cpr::SslOptions sslOpts = cpr::Ssl(cpr::ssl::CaPath{caCertFile.string()});
    cpr::Response r = cpr::Get(cpr::Url{fileUrl}, sslOpts);

    if(r.status_code == 0)
    {
        std::cerr << r.error.message << std::endl;
        return false;
    }
    else if (r.status_code >= 400)
    {
        std::cerr << "Error [" << r.status_code << "] making request" << std::endl;
        return false;
    } else
    {
        std::cout << "Status: " << r.status_code << std::endl;
        std::cout << "Request took: " << r.elapsed << std::endl;

        // Write to file
        std::ofstream ofs(destination);
        ofs << r.text; 
        ofs.close();
    }
    return true;
}