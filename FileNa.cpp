#include <iostream>             
#include <fstream>               
#include <sstream>              
#include <vector>                
#include <string>                
#include <unordered_map>         
#include <queue>                 
#include <stack>                
#include <set>                   
#include <chrono>               
#include <limits>                
#include <cmath>                 
#include <algorithm>             


struct Node {
    std::string id;  
    double lon;     
    double lat;      

    // Список смежных вершин
    std::vector<std::pair<Node*, double>> neighbors;

    // Конструктор
    Node(const std::string& _id, double _lon, double _lat)
        : id(_id), lon(_lon), lat(_lat) {}
};

struct Graph {
    //ключ:строковый id вершины (например, "30.376882,59.851314")
    //значение: указатель на динамически созданный объект Node
    std::unordered_map<std::string, Node*> nodes;

    void parse_line(const std::string& line) {
        
        auto pos_colon = line.find(':');
        if (pos_colon == std::string::npos) {
            return;
        }

        // Родительская часть строки
        std::string parent_str = line.substr(0, pos_colon);
        // Дочерняя часть
        std::string children_str = line.substr(pos_colon + 1);

        // Парсим родительскую вершину (lon1, lat1)
        double plon = 0.0, plat = 0.0;
        {
            auto comma_pos = parent_str.find(',');
            if (comma_pos == std::string::npos) {
                return; // неправильный формат
            }
            std::string lon_str = parent_str.substr(0, comma_pos);
            std::string lat_str = parent_str.substr(comma_pos + 1);

            plon = std::stod(lon_str);   // переводим строку в double (долгота)
            plat = std::stod(lat_str);   // широта
        }

        std::string parent_id = parent_str;

        // Ищем родительскую вершину (Node)
        Node* parent_node = nullptr;
        auto it_p = nodes.find(parent_id);
        if (it_p != nodes.end()) {
            parent_node = it_p->second;  // уже есть
        }
        else {
            // создаём новую
            parent_node = new Node(parent_id, plon, plat);
            nodes[parent_id] = parent_node;
        }

        // Разделяем дочернюю часть по ';'
        std::vector<std::string> child_parts;
        {
            std::istringstream iss(children_str);
            std::string token;
            while (std::getline(iss, token, ';')) {
                if (!token.empty()) {
                    child_parts.push_back(token);
                }
            }
        }

        // Обрабатываем каждого "ребёнка" вида "lon2,lat2,weight2"
        for (auto& child : child_parts) {
            // Ищем первую и вторую запятые
            auto first_comma = child.find(',');
            if (first_comma == std::string::npos) {
                continue;
            }
            auto second_comma = child.find(',', first_comma + 1);
            if (second_comma == std::string::npos) {
                continue;
            }

            // Извлекаем строки для lon2, lat2, weight2
            std::string lon_str = child.substr(0, first_comma);
            std::string lat_str = child.substr(first_comma + 1, second_comma - (first_comma + 1));
            std::string w_str = child.substr(second_comma + 1);

            // Преобразуем в double
            double clon = std::stod(lon_str);
            double clat = std::stod(lat_str);
            double cweight = std::stod(w_str);

            std::string child_id = lon_str + "," + lat_str;

            // Ищем создаём дочернюю вершину
            Node* child_node = nullptr;
            auto it_c = nodes.find(child_id);
            if (it_c != nodes.end()) {
                child_node = it_c->second;
            }
            else {
                child_node = new Node(child_id, clon, clat);
                nodes[child_id] = child_node;
            }


            // 1) Проверяем, нет ли уже child_node среди соседей parent_node
            bool found_in_parent = false;
            for (auto& p : parent_node->neighbors) {
                if (p.first->id == child_id) {
                    p.second = cweight;  // Обновляем вес
                    found_in_parent = true;
                    break;
                }
            }
            if (!found_in_parent) {
                parent_node->neighbors.emplace_back(child_node, cweight);
            }

            // 2) Аналогично для обратной связи (child_node->neighbors)
            bool found_in_child = false;
            for (auto& p : child_node->neighbors) {
                if (p.first->id == parent_id) {
                    p.second = cweight;
                    found_in_child = true;
                    break;
                }
            }
            if (!found_in_child) {
                child_node->neighbors.emplace_back(parent_node, cweight);
            }
        }
    }

    void get_graph(const std::string& file_name) {
        std::ifstream file(file_name);
        if (!file.is_open()) {
            std::cerr << "Error: cannot open file " << file_name << std::endl;
            return;
        }
        std::string line;
        // Считываем файл построчно
        while (std::getline(file, line)) {
            if (!line.empty()) {
                parse_line(line);
            }
        }
        file.close();
    }

    // Поиск ближайшей вершины к заданным координатам (lon, lat)
    Node* find_closest(double lon, double lat) {
        double min_distance = std::numeric_limits<double>::max();
        Node* node_founded = nullptr;

        // Проходим по всем вершинам (O(N)), где N ~ количество вершин
        for (auto& pair : nodes) {
            Node* node = pair.second;
            // Евклидово расстояние
            double dx = node->lon - lon;
            double dy = node->lat - lat;
            double dist = dx * dx + dy * dy;

            if (dist < min_distance) {
                min_distance = dist;
                node_founded = node;
            }
        }
        return node_founded;
    }

    // Поиск в глубину (DFS)
    std::unordered_map<std::string, double> dfs(Node* start) {
        std::stack<std::string> st;
        std::unordered_map<std::string, bool> visited;
        std::unordered_map<std::string, double> dist;

        st.push(start->id);
        visited[start->id] = true;
        dist[start->id] = 0.0;

        while (!st.empty()) {
            std::string cur_id = st.top();
            st.pop();

            Node* cur_node = nodes[cur_id];
            // Перебираем соседей
            for (auto& neighb : cur_node->neighbors) {
                Node* neighb_node = neighb.first;
                if (!visited[neighb_node->id]) {
                    visited[neighb_node->id] = true;
                    st.push(neighb_node->id);
                    // Увеличиваем dist на 1 (т.к. считаем рёбра за 1)
                    dist[neighb_node->id] = dist[cur_id] + 1.0;
                }
            }
        }
        return dist;
    }

    // Поиск в ширину (BFS)
    std::unordered_map<std::string, double> bfs(Node* start) {
        std::queue<std::string> q;
        std::unordered_map<std::string, bool> visited;
        std::unordered_map<std::string, double> dist;

        q.push(start->id);
        visited[start->id] = true;
        dist[start->id] = 0.0;

        while (!q.empty()) {
            std::string cur_id = q.front();
            q.pop();

            Node* cur_node = nodes[cur_id];
            for (auto& neighb : cur_node->neighbors) {
                Node* neighb_node = neighb.first;
                if (!visited[neighb_node->id]) {
                    visited[neighb_node->id] = true;
                    q.push(neighb_node->id);
                    dist[neighb_node->id] = dist[cur_id] + 1.0;
                }
            }
        }
        return dist;
    }

    // Алгоритм Дейкстры
    std::unordered_map<std::string, double> dijkstra(Node* start) {
        std::unordered_map<std::string, double> dist;
        dist.reserve(nodes.size());

        for (auto& kv : nodes) {
            dist[kv.first] = std::numeric_limits<double>::infinity();
        }
        dist[start->id] = 0.0;

        // set для (distance, Node*)
        std::set<std::pair<double, Node*>> pq;
        pq.insert({ 0.0, start });

        while (!pq.empty()) {
            // Берём вершину с минимальным расстоянием
            auto top_it = pq.begin();
            double cur_dist = top_it->first;
            Node* cur_node = top_it->second;
            pq.erase(top_it);

            // Если извлечённая dist > той, что уже в массиве dist, пропускаем
            if (cur_dist > dist[cur_node->id]) {
                continue;
            }

            // Перебираем всех соседей
            for (auto& edge : cur_node->neighbors) {
                Node* v = edge.first;
                double w = edge.second;
                double candidate = dist[cur_node->id] + w;
                if (candidate < dist[v->id]) {
                    // Удаляем старую запись (dist[v->id], v) из pq (если была)
                    auto it = pq.find({ dist[v->id], v });
                    if (it != pq.end()) {
                        pq.erase(it);
                    }
                    dist[v->id] = candidate;
                    pq.insert({ candidate, v });
                }
            }
        }
        return dist;
    }

    // Эвристика: Евклидово расстояние (lon/lat)
    double heuristic(Node* a, Node* b) {
        double dx = a->lon - b->lon;
        double dy = a->lat - b->lat;
        return std::sqrt(dx * dx + dy * dy);
    }

    
    // Алгоритм A*: ищем кратчайший путь от start до goal, возвращаем сам путь (список Node*)
    std::vector<Node*> a_star(Node* start, Node* goal) {
        // gScore[u] = лучший известный путь от start до u
        // fScore[u] = gScore[u] + heuristic(u, goal)
        std::unordered_map<Node*, double> gScore;
        std::unordered_map<Node*, double> fScore;
        std::unordered_map<Node*, Node*> cameFrom;

        gScore.reserve(nodes.size());
        fScore.reserve(nodes.size());

        // Изначально у всех = бесконечность
        for (auto& kv : nodes) {
            Node* n = kv.second;
            gScore[n] = std::numeric_limits<double>::infinity();
            fScore[n] = std::numeric_limits<double>::infinity();
        }

        // Стартовая вершина: расстояние = 0
        gScore[start] = 0.0;
        // fScore = 0 + эвристика
        fScore[start] = heuristic(start, goal);

        // Описываем функцию сравнения для очереди
        auto cmp = [](const std::pair<double, Node*>& a, const std::pair<double, Node*>& b) {
            return a.first > b.first;
            };
        std::priority_queue<std::pair<double, Node*>,
            std::vector<std::pair<double, Node*>>,
            decltype(cmp)> openSet(cmp);

        // Кладём старт в очередь
        openSet.push({ fScore[start], start });

        while (!openSet.empty()) {
            // Извлекаем вершину с наименьшим fScore
            Node* current = openSet.top().second;
            openSet.pop();

            // Если добрались до goal, восстанавливаем путь
            if (current == goal) {
                std::vector<Node*> path;
                Node* tmp = current;
                while (tmp) {
                    path.push_back(tmp);
                    if (cameFrom.find(tmp) == cameFrom.end()) {
                        break;
                    }
                    tmp = cameFrom[tmp];
                }
                std::reverse(path.begin(), path.end());
                return path;
            }

            // Перебираем соседей
            for (auto& edge : current->neighbors) {
                Node* neighbor = edge.first;
                double w = edge.second;

                double tentative_g = gScore[current] + w;
                if (tentative_g < gScore[neighbor]) {
                    cameFrom[neighbor] = current;
                    gScore[neighbor] = tentative_g;
                    fScore[neighbor] = tentative_g + heuristic(neighbor, goal);
                    openSet.push({ fScore[neighbor], neighbor });
                }
            }
        }

        // Путь не найден
        return {};
    }
};

int main() {
    Graph graph;
    graph.get_graph("spb_graph3.txt");
    std::cout << "Graph complete\n";

    double lon_start = 30.376882;
    double lat_start = 59.851314;
    double lon_end = 30.308108;
    double lat_end = 59.957238;

    Node* start = graph.find_closest(lon_start, lat_start);
    Node* end = graph.find_closest(lon_end, lat_end);

    if (!start || !end) {
        std::cout << "Could not find start or end nodes.\n";
        return 1;
    }
    std::cout << "Start node: " << start->id << "\n";
    std::cout << "End node: " << end->id << "\n";
    //DFS
    {
        auto start_t = std::chrono::high_resolution_clock::now();
        auto res_dfs = graph.dfs(start);
        auto end_t = std::chrono::high_resolution_clock::now();
        auto dur_dfs = std::chrono::duration_cast<std::chrono::microseconds>(end_t - start_t).count();

        std::cout << "DFS time: " << dur_dfs << " microseconds\n";
        if (res_dfs.find(end->id) != res_dfs.end()) {
            std::cout << "DFS dist to goal: " << res_dfs[end->id] << "\n";
        }
        else {
            std::cout << "DFS: goal not reachable\n";
        }
    }

   
    //BFS
    {
        auto start_t = std::chrono::high_resolution_clock::now();
        auto res_bfs = graph.bfs(start);
        auto end_t = std::chrono::high_resolution_clock::now();
        auto dur_bfs = std::chrono::duration_cast<std::chrono::microseconds>(end_t - start_t).count();

        std::cout << "BFS time: " << dur_bfs << " microseconds\n";
        if (res_bfs.find(end->id) != res_bfs.end()) {
            std::cout << "BFS dist to goal: " << res_bfs[end->id] << "\n";
        }
        else {
            std::cout << "BFS: goal not reachable\n";
        }
    }

    //Dijkstra
    {
        auto start_t = std::chrono::high_resolution_clock::now();
        auto dist_map = graph.dijkstra(start);
        auto end_t = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end_t - start_t).count();

        std::cout << "Dijkstra time: " << dur << " microseconds\n";
        if (dist_map.find(end->id) != dist_map.end()) {
            std::cout << "Dijkstra dist to goal: " << dist_map[end->id] << "\n";
        }
        else {
            std::cout << "Dijkstra: goal not reachable\n";
        }
    }

    //A*
    {
        auto start_t = std::chrono::high_resolution_clock::now();
        auto path = graph.a_star(start, end);
        auto end_t = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end_t - start_t).count();

        std::cout << "A* time: " << dur << " microseconds\n";
        if (!path.empty()) {
            // Если путь найден, посчитаем суммарный вес
            double total_weight = 0.0;
            for (size_t i = 0; i + 1 < path.size(); ++i) {
                Node* cur = path[i];
                Node* nxt = path[i + 1];
                // Ищем вес ребра (cur -> nxt)
                double edge_weight = 0.0;
                bool found = false;
                for (auto& edge : cur->neighbors) {
                    if (edge.first == nxt) {
                        edge_weight = edge.second;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    total_weight += edge_weight;
                }
            }
            std::cout << "A* path found! nodes: " << path.size()
                << ", total weight = " << total_weight << "\n";
        }
        else {
            std::cout << "A*: goal not reachable\n";
        }
    }

    return 0;
}
