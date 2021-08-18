#include <network-monitor/transport-network.h>

#include <algorithm>

using NetworkMonitor::TransportNetwork;

namespace
{
    // Helper function for finding pointer objects in a vector
    template <typename T>
    struct pointer_values_equal
    {
        const T* to_find;

        bool operator()(const T* other) const
        {
            return *to_find == *other;
        }
    };
}

bool NetworkMonitor::Station::operator== (const Station& other) const
{
    return id == other.id;
}

bool NetworkMonitor::Line::operator== (const Line& other) const
{
    return id == other.id;
}

bool NetworkMonitor::Route::operator== (const Route& other) const
{
    return id == other.id;
}

TransportNetwork::TransportNetwork() {}

TransportNetwork::~TransportNetwork() {}

bool TransportNetwork::AddStation(const Station& station)
{
    // Check we're not adding a station with the same id
    pointer_values_equal<Station> station_ptr_finder { &station };
    if ( std::find_if(stations_.begin(), stations_.end(), station_ptr_finder) != stations_.end() )
    {
        return false;
    }

    stations_.push_back(&station);
    // TODO: Update this to use a shared ptr
    GraphNode* stationNode = new GraphNode{station, std::map<Id, GraphEdge*> {}, 0};
    stationGraph_.insert(std::pair<Id, GraphNode*>{station.id, stationNode});
    return true;
}

bool TransportNetwork::AddLine(const Line& line)
{
    // Check we're not adding a line with the same id
    pointer_values_equal<Line> line_ptr_finder { &line };
    if ( std::find_if(lines_.begin(), lines_.end(), line_ptr_finder) != lines_.end() )
    {
        return false;
    }

    for (const auto route : line.routes)
    {
        // Check we're not adding a route with the same id
        pointer_values_equal<Route> route_ptr_finder { &route };
        if ( std::find_if(routes_.begin(), routes_.end(), route_ptr_finder) != routes_.end() )
        {
            continue;
        }
        routes_.push_back(&route);
        AddRoute(route);
    }
    
    lines_.push_back(&line);
    return true;
}

void TransportNetwork::AddRoute(const Route& route)
{
    for (const auto stationId : route.stops)
    {
        auto edges = stationGraph_[stationId]->edges;
        if (edges.find(stationId) == edges.end())
        {
            GraphEdge* newEdge = new GraphEdge{
                std::vector<Id>{ route.id },
                stationGraph_[stationId],
                0
            };
            edges[stationId] = newEdge;
        }
        else
        {
            edges[stationId]->routeIds.push_back(route.id);
        }
    }
}