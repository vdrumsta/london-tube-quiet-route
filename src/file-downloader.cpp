#include <network-monitor/file-downloader.h>

#include <cpr/cpr.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <iostream>
#include <fstream>

using json = nlohmann::json;

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

nlohmann::json NetworkMonitor::ParseJsonFile(const std::filesystem::path& source)
{
    nlohmann::json parsed {};
    if (!std::filesystem::exists(source)) {
        return parsed;
    }
    try {
        std::ifstream file {source};
        file >> parsed;
    } catch (...) {
        // Will return an empty object.
    }
    return parsed;
}