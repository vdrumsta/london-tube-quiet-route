#include <network-monitor/transport-network.h>

#include <algorithm>

using NetworkMonitor::TransportNetwork;

TransportNetwork::TransportNetwork() {}

TransportNetwork::~TransportNetwork() {}

bool TransportNetwork::AddStation(const Station& station)
{
    // Check we're not adding a station with the same name or id
    if ( std::any_of(stations_.begin(), stations_.end(),
            [&station](const Station* i){
                return i->id == station.id;
            }) )
    {
        return false;
    }

    stations_.push_back(&station);
    return true;
}