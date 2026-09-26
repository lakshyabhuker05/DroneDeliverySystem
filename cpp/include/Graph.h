#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <limits>
#include <algorithm>

/*******************************************************************************
 * Graph
 * Weighted, undirected graph representing the city / delivery-zone network.
 * Nodes  = cities (delivery hubs / destinations)
 * Edges  = direct flight corridors between cities, weight = distance (km)
 *
 * Implements Dijkstra's Algorithm (using a binary min-heap via
 * std::priority_queue) to compute the shortest route + distance between any
 * two cities, which DeliveryManager then converts into an ETA using the
 * assigned drone's speed.
 ******************************************************************************/
class Graph {
private:
    std::unordered_map<std::string, int> cityIndex;
    std::vector<std::string> cityNames;
    std::vector<std::vector<std::pair<int, double>>> adjacency; // adjacency list: node -> (neighbor, weight)

    int indexOf(const std::string& city) const {
        auto it = cityIndex.find(city);
        return it == cityIndex.end() ? -1 : it->second;
    }

public:
    void addCity(const std::string& name) {
        if (cityIndex.count(name)) return;
        cityIndex[name] = static_cast<int>(cityNames.size());
        cityNames.push_back(name);
        adjacency.emplace_back();
    }

    void addRoute(const std::string& from, const std::string& to, double distanceKm) {
        addCity(from);
        addCity(to);
        int u = indexOf(from), v = indexOf(to);
        adjacency[u].push_back({v, distanceKm});
        adjacency[v].push_back({u, distanceKm}); // undirected corridor
    }

    bool hasCity(const std::string& name) const { return cityIndex.count(name) > 0; }

    std::vector<std::string> allCities() const { return cityNames; }

    struct RouteResult {
        bool found = false;
        double distanceKm = 0.0;
        std::vector<std::string> path;
    };

    // Dijkstra's shortest path algorithm: O((V + E) log V) using a min-heap.
    RouteResult shortestRoute(const std::string& source, const std::string& destination) const {
        RouteResult result;
        int src = indexOf(source), dst = indexOf(destination);
        if (src == -1 || dst == -1) return result;

        size_t n = cityNames.size();
        std::vector<double> dist(n, std::numeric_limits<double>::infinity());
        std::vector<int> prev(n, -1);
        std::vector<bool> visited(n, false);

        using PDI = std::pair<double, int>; // (distance, node)
        std::priority_queue<PDI, std::vector<PDI>, std::greater<PDI>> minHeap;

        dist[src] = 0.0;
        minHeap.push({0.0, src});

        while (!minHeap.empty()) {
            auto [d, u] = minHeap.top();
            minHeap.pop();
            if (visited[u]) continue;
            visited[u] = true;
            if (u == dst) break;

            for (const auto& edge : adjacency[u]) {
                int v = edge.first;
                double w = edge.second;
                if (!visited[v] && dist[u] + w < dist[v]) {
                    dist[v] = dist[u] + w;
                    prev[v] = u;
                    minHeap.push({dist[v], v});
                }
            }
        }

        if (dist[dst] == std::numeric_limits<double>::infinity()) return result;

        result.found = true;
        result.distanceKm = dist[dst];
        std::vector<int> pathIdx;
        for (int at = dst; at != -1; at = prev[at]) pathIdx.push_back(at);
        std::reverse(pathIdx.begin(), pathIdx.end());
        for (int idx : pathIdx) result.path.push_back(cityNames[idx]);
        return result;
    }
};

#endif // GRAPH_H
