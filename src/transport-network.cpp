#include <network-monitor/transport-network.h>

#include <algorithm>
#include <stdexcept>

using NetworkMonitor::TransportNetwork;
using Id = std::string;

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

TransportNetwork::TransportNetwork() = default;
TransportNetwork::~TransportNetwork() = default;

TransportNetwork::TransportNetwork(
    const TransportNetwork& copied
) = default;
TransportNetwork::TransportNetwork(
    TransportNetwork&& moved
) = default;
TransportNetwork& TransportNetwork::operator=(
    const TransportNetwork& copied
) = default;
TransportNetwork& TransportNetwork::operator=(
    TransportNetwork&& moved
) = default;

bool TransportNetwork::AddStation(const Station& station)
{
    // Check we're not adding a station with the same id
    if ( stations_.find(station.id) != stations_.end() ) return false;

    std::shared_ptr<GraphNode> stationNode{new GraphNode{station, std::map<Id, std::shared_ptr<GraphEdge>> {}, {}, 0}};
    stations_.insert(std::pair<Id, std::shared_ptr<GraphNode>>{station.id, stationNode});
    return true;
}

bool TransportNetwork::AddLine(const Line& line)
{
    if ( lines_.find(line.id) != lines_.end() ) return false;
    
    // Check all stations are in the network
    if ( std::none_of(line.routes.begin(),
                        line.routes.end(),
                        [&](const Route& route){
                            return IsRouteStopsExist(route);}) )
    {
        return false;
    }

    lines_[line.id] = std::make_shared<LineInternal>(LineInternal{
        line.id,
        line.name,
        {}  // std::map<Id, std::shared_ptr<RouteInternal>> routes
    });

    for (auto& route : line.routes)
    {
        // Check we're not adding a route with the same id
        if ( routes_.find(route.id) != routes_.end() )
        {
            return false;
        }
        AddRouteToLine(route, lines_[line.id]);
    }
    
    return true;
}

void TransportNetwork::AddRouteToLine(const Route& route, std::shared_ptr<LineInternal>& line)
{
    // Construct a vector of station nodes
    std::vector<std::shared_ptr<GraphNode>> routeStops {};
    for (const auto& stop : route.stops)
    {
        // TODO: see if you can use transform for this
        routeStops.push_back(stations_[stop]);
    }

    // Construct edges between station nodes
    for (int stopIndex = 0; stopIndex < route.stops.size() - 1; ++stopIndex)
    {
        const Id& currentStop = route.stops[stopIndex];
        const Id& nextStop = route.stops[stopIndex + 1];
        AddEdge(currentStop, nextStop, route.id);
    }

    routes_[route.id] = std::make_shared<RouteInternal>(RouteInternal{
        route.id,
        line,
        routeStops
    });
}

void TransportNetwork::AddEdge(
    const Id& stationA,
    const Id& stationB,
    const Id& routeId
)
{
    auto& stationAOutboundEdges = stations_[stationA]->outboundEdges;
    auto& stationBInboundEdges = stations_[stationB]->inboundEdges;

    AddRouteToEdge(stationAOutboundEdges, stationB, routeId);
    AddRouteToEdge(stationBInboundEdges, stationA, routeId);
}

void TransportNetwork::AddRouteToEdge(
    std::map<Id, std::shared_ptr<GraphEdge>>& edges,
    const Id& stationId,
    const Id& routeId
)
{
    if (edges.find(stationId) == edges.end())
    {
        std::shared_ptr<GraphEdge> newEdge{new GraphEdge{
            std::vector<Id>{ routeId },
            stations_[stationId],
            0,  // travelTime
        }};
        edges[stationId] = newEdge;
    }
    else
    {
        edges[stationId]->routeIds.push_back(routeId);
    }
}

long long int TransportNetwork::GetPassengerCount(const Id& station)
{
    if (stations_.find(station) == stations_.end())
    {
        throw std::runtime_error("Station " + station + " not found.");
    }

    return stations_.[station]->passengerCount;
}

bool TransportNetwork::RecordPassengerEvent(const PassengerEvent& event)
{
    if (stations_.find(event.stationId) == stations_.end())
        return false;
    if (event.type != PassengerEvent::Type::In and event.type != PassengerEvent::Type::Out)
        return false;

    int passengerNum = event.type == PassengerEvent::Type::In ? 1 : -1;
    stations_[event.stationId]->passengerCount += passengerNum;
    return true;
}

std::vector<Id> TransportNetwork::GetRoutesServingStation(const Id& station)
{
    std::vector<Id> foundRouteIds{};
    const auto& outboundEdges = stations_[station]->outboundEdges;
    const auto& inboundEdges = stations_[station]->inboundEdges;

    AddUniqueEdgeRoutes(outboundEdges, foundRouteIds);
    AddUniqueEdgeRoutes(inboundEdges, foundRouteIds);
    
    return foundRouteIds;
}

void TransportNetwork::AddUniqueEdgeRoutes(
    const std::map<Id, std::shared_ptr<GraphEdge>>& edges,
    std::vector<Id>& oFoundRoutes
)
{
    for (const auto& [stationKey, edge] : edges)
    {
        for (const auto& routeId : edge->routeIds)
        {
            if (std::find(oFoundRoutes.begin(), oFoundRoutes.end(), routeId) == oFoundRoutes.end())
            {
                oFoundRoutes.push_back(routeId);
            }
        }
    }
}

unsigned int TransportNetwork::GetTravelTime(const Id& stationA, const Id& stationB)
{
    if (stationA == stationB) return 0;

    // Check outbound edges
    auto& stationAOutEdges = stations_[stationA]->outboundEdges;
    if (stationAOutEdges.find(stationB) != stationAOutEdges.end())
    {
        return stationAOutEdges[stationB]->travelTime;
    }

    // Check inbound edges
    auto& stationAInEdges = stations_[stationA]->inboundEdges;
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
        auto stationANode = stations_[stationA];
        auto stationBNode = stations_[stationB];
        auto routeObj = routes_[route];

        auto currentStopItr = std::find(routeObj->stops.begin(), routeObj->stops.end(), stationANode);
        auto stopEndItr = std::find(routeObj->stops.begin(), routeObj->stops.end(), stationBNode);

        for (; currentStopItr != stopEndItr; ++currentStopItr)
        {
            Id currentStopId = (*(currentStopItr))->station.id;
            Id nextStopId = (*(std::next(currentStopItr)))->station.id;
            travelTimeSum += GetTravelTime(currentStopId, nextStopId);
        }
    }

    return travelTimeSum;
}

int TransportNetwork::GetStopNumberOnRoute(const Id& stationId, const Id& route)
{
    auto routeStops = routes_[route]->stops;
    auto stationNode = stations_[stationId];
    auto stopItr = std::find(routeStops.begin(), routeStops.end(), stationNode);
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

std::shared_ptr<TransportNetwork::GraphEdge> TransportNetwork::GetEdge(
        const Id& stationA,
        const Id& stationB,
        const GraphEdge::Direction& direction
)
{
    if (direction == GraphEdge::Direction::Inbound)
    {
        auto& inboundEdges = stations_[stationA]->inboundEdges;
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
        auto& outboundEdges = stations_[stationA]->outboundEdges;
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
    auto& inboundEdges = stations_[stationA]->inboundEdges;
    auto& outboundEdges = stations_[stationA]->outboundEdges;

    return inboundEdges.find(stationB) != inboundEdges.end()
            or outboundEdges.find(stationB) != outboundEdges.end();
}

bool TransportNetwork::IsRouteStopsExist(const Route& route)
{
    return std::all_of(route.stops.begin(),
                        route.stops.end(),
                        [&](const Id& stop){
                            return stations_.find(stop) != stations_.end();
                        });
}