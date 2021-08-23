#include <network-monitor/transport-network.h>

#include <algorithm>
#include <stdexcept>

using NetworkMonitor::TransportNetwork;
using Id = std::string;

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
    GraphNode* stationNode = new GraphNode{station, std::map<Id, GraphEdge*> {}, {}, 0};
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

    for (auto& route : line.routes)
    {
        // Check we're not adding a route with the same id
        if ( routes_.find(route.id) != routes_.end() )
        {
            continue;
        }
        AddRoute(route);
    }
    
    lines_.push_back(&line);
    return true;
 }

void TransportNetwork::AddRoute(const Route& route)
{
    routes_[route.id] = &route;

    for (int stopIndex = 0; stopIndex < route.stops.size() - 1; ++stopIndex)
    {
        const Id& currentStop = route.stops[stopIndex];
        const Id& nextStop = route.stops[stopIndex + 1];
        AddEdge(currentStop, nextStop, route.id, GraphEdge::Direction::Outbound);
        AddEdge(nextStop, currentStop, route.id, GraphEdge::Direction::Inbound);
    }
}

void TransportNetwork::AddEdge(
    const Id& stationA,
    const Id& stationB,
    const Id& routeId,
    const GraphEdge::Direction& direction
)
{
    std::map<Id, GraphEdge*> edges;
    if (direction == GraphEdge::Direction::Inbound)
    {
        edges = stationGraph_[stationA]->inboundEdges;
    }
    {
        edges = stationGraph_[stationA]->outboundEdges;
    }

    if (edges.find(stationB) == edges.end())
    {
        GraphEdge* newEdge = new GraphEdge{
            std::vector<Id>{ routeId },
            stationGraph_[stationB],
            0,  // travelTime
        };
        edges[stationB] = newEdge;
    }
    else
    {
        edges[stationB]->routeIds.push_back(routeId);
    }

    //
    if (direction == GraphEdge::Direction::Inbound)
    {
        stationGraph_[stationA]->inboundEdges = edges;
    }
    {
        stationGraph_[stationA]->outboundEdges = edges;
    }
}

long long int TransportNetwork::GetPassengerCount(const Id& station)
{
    if (stationGraph_.find(station) == stationGraph_.end())
    {
        throw std::runtime_error("Station " + station + " not found.");
    }
    
    return stationGraph_[station]->passengerCount;
}

bool TransportNetwork::RecordPassengerEvent(const PassengerEvent& event)
{
    if (stationGraph_.find(event.stationId) == stationGraph_.end())
        return false;
    if (event.type != PassengerEvent::Type::In and event.type != PassengerEvent::Type::Out)
        return false;

    int passengerNum = event.type == PassengerEvent::Type::In ? 1 : -1;
    stationGraph_[event.stationId]->passengerCount += passengerNum;
    return true;
}

std::vector<Id> TransportNetwork::GetRoutesServingStation(const Id& station)
{
    std::vector<Id> routeIds{};
    const auto outboundEdges = stationGraph_[station]->outboundEdges;
    for (const auto& [stationKey, edge] : outboundEdges)
    {
        for (const auto& routeId : edge->routeIds)
        {
            if (std::find(routeIds.begin(), routeIds.end(), routeId) == routeIds.end())
            {
                routeIds.push_back(routeId);
            }
        }
    }

    return routeIds;
}

unsigned int TransportNetwork::GetTravelTime(const Id& stationA, const Id& stationB)
{
    if (stationA == stationB) return 0;

    // Check outbound edges
    auto& stationAOutEdges = stationGraph_[stationA]->outboundEdges;
    if (stationAOutEdges.find(stationB) != stationAOutEdges.end())
    {
        return stationAOutEdges[stationB]->travelTime;
    }

    // Check inbound edges
    auto& stationAInEdges = stationGraph_[stationA]->inboundEdges;
    if (stationAInEdges.find(stationB) != stationAInEdges.end())
    {
        return stationAInEdges[stationB]->travelTime;
    }

    return 0;
}

unsigned int TransportNetwork::GetTravelTime(
    const Id& line,
    const Id& route,
    const Id& stationA,
    const Id& stationB
)
{
    if (stationA == stationB) return 0;
    
    int travelTimeSum{0};
    auto stopDirection = GetDirectionBetweenStops(stationA, stationB, route);
    if (stopDirection == GraphEdge::Direction::Outbound)
    {
        auto routeObj = routes_[route];
        auto currentStopItr = std::find(routeObj->stops.begin(), routeObj->stops.end(), stationA);
        auto stopEndItr = std::find(routeObj->stops.begin(), routeObj->stops.end(), stationB);

        for (; currentStopItr != stopEndItr; ++currentStopItr)
        {
            auto nextStopId = *(std::next(currentStopItr));
            travelTimeSum += GetTravelTime(*(currentStopItr), nextStopId);
        }
    }

    return travelTimeSum;
}

int TransportNetwork::GetStopNumberOnRoute(const Id& stationId, const Id& route)
{
    auto routeStops = routes_[route]->stops;
    auto stopItr = std::find(routeStops.begin(), routeStops.end(), stationId);
    if (stopItr == routeStops.end())
    {
        return 0;   // Stop is not on route
    }

    return std::distance(routeStops.begin(), stopItr);
}

TransportNetwork::GraphEdge::Direction TransportNetwork::GetDirectionBetweenStops(
    const Id& stationA,
    const Id& stationB,
    const Id& route
)
{
    int stopANumber = GetStopNumberOnRoute(stationA, route);
    int stopBNumber = GetStopNumberOnRoute(stationB, route);

    return stopANumber - stopBNumber > 0 ? GraphEdge::Direction::Inbound : GraphEdge::Direction::Outbound;
}

bool TransportNetwork::SetTravelTime(const Id& stationA, const Id& stationB, const unsigned int travelTime)
{
    if (!IsEdgeExists(stationA, stationB)) return false;

    auto stationAInbound = GetEdge(stationA, stationB, GraphEdge::Direction::Inbound);
    auto stationAOutbound = GetEdge(stationA, stationB, GraphEdge::Direction::Outbound);
    auto stationBInbound = GetEdge(stationB, stationA, GraphEdge::Direction::Inbound);
    auto stationBOutbound = GetEdge(stationB, stationA, GraphEdge::Direction::Outbound);

    if (stationAInbound != nullptr) stationAInbound->travelTime = travelTime;
    if (stationAOutbound != nullptr) stationAOutbound->travelTime = travelTime;
    if (stationBInbound != nullptr) stationBInbound->travelTime = travelTime;
    if (stationBOutbound != nullptr) stationBOutbound->travelTime = travelTime;

    return true;
}

TransportNetwork::GraphEdge* TransportNetwork::GetEdge(
        const Id& stationA,
        const Id& stationB,
        const GraphEdge::Direction& direction
)
{
    if (direction == GraphEdge::Direction::Inbound)
    {
        auto& inboundEdges = stationGraph_[stationA]->inboundEdges;
        if (inboundEdges.find(stationB) == inboundEdges.end())
        {
            return nullptr;
        }
        else
        {
            return inboundEdges[stationB];
        }
    }
    else
    {
        auto& outboundEdges = stationGraph_[stationA]->outboundEdges;
        if (outboundEdges.find(stationB) == outboundEdges.end())
        {
            return nullptr;
        }
        else
        {
            return outboundEdges[stationB];
        }
    }
}

bool TransportNetwork::IsEdgeExists(
    const Id& stationA,
    const Id& stationB
)
{
    auto& inboundEdges = stationGraph_[stationA]->inboundEdges;
    auto& outboundEdges = stationGraph_[stationA]->outboundEdges;

    return inboundEdges.find(stationB) != inboundEdges.end()
            or outboundEdges.find(stationB) != outboundEdges.end();
}