/*
===============================================================================
    Nombre:    Santiago Arango Henao
    Curso:     Programación Paralela
    Entrega Parcial 2
===============================================================================
*/

#include <bits/stdc++.h>
using namespace std;

const string GOAL = "ABCDEFGHIJKLMNO#";

// -------------------- utilidades: movimientos / vecinos ---------------------
struct Move { int dr, dc; const char* name; };
static const Move MOVES[4] = {
    {-1, 0, "UP"},
    {+1, 0, "DOWN"},
    { 0,-1, "LEFT"},
    { 0,+1, "RIGHT"},
};

// llena `out` con pares (estado_resultante, accion_string)
static inline void vecinos(const string& s, vector<pair<string,string>>& out) {
    out.clear();
    int pos = (int)s.find('#');
    int r = pos / 4, c = pos % 4;
    for (int k = 0; k < 4; ++k) {
        int nr = r + MOVES[k].dr;
        int nc = c + MOVES[k].dc;
        if (nr >= 0 && nr < 4 && nc >= 0 && nc < 4) {
            string t = s;
            int np = nr * 4 + nc;
            swap(t[pos], t[np]);
            out.emplace_back(t, string(MOVES[k].name));
        }
    }
}

// versión que devuelve solo estados (útil en BFS original)
inline vector<pair<string,string>> obtenerVecinos(const string& state) {
    vector<pair<string,string>> neigh;
    vecinos(state, neigh);
    return neigh;
}

// -------------------- heurísticas ------------------------------------------
static inline int h1_misplaced(const string& s) {
    int c = 0;
    for (int i = 0; i < 16; ++i)
        if (s[i] != '#' && s[i] != GOAL[i]) ++c;
    return c;
}

static int goal_r[256], goal_c[256];
static void init_goal_pos() {
    for (int i = 0; i < 256; ++i) { goal_r[i] = goal_c[i] = -1; }
    for (int i = 0; i < 16; ++i) {
        unsigned char ch = (unsigned char)GOAL[i];
        goal_r[ch] = i / 4;
        goal_c[ch] = i % 4;
    }
}

static inline int h2_manhattan(const string& s) {
    int sum = 0;
    for (int i = 0; i < 16; ++i) {
        char ch = s[i];
        if (ch == '#') continue;
        int r = i / 4, c = i % 4;
        int gr = goal_r[(unsigned char)ch], gc = goal_c[(unsigned char)ch];
        // seguridad: en caso de que goal_r no esté bien inicializado, evitamos UB
        if (gr >= 0 && gc >= 0)
            sum += abs(r - gr) + abs(c - gc);
    }
    return sum;
}

// -------------------- comprobación de solucionabilidad ----------------------
static inline int inversions(const string& s) {
    string t; t.reserve(16);
    for (char ch : s) if (ch != '#') t.push_back(ch);
    int inv = 0;
    for (int i = 0; i < (int)t.size(); ++i)
        for (int j = i + 1; j < (int)t.size(); ++j)
            if (t[i] > t[j]) ++inv;
    return inv;
}

static inline bool solvable(const string& s) {
    int inv = inversions(s);
    int blank = (int)s.find('#');
    int rowFromBottom = 4 - (blank / 4); // 1..4 (1 = última fila)
    // Regla estándar para ancho par (4): (inversions + rowFromBottom) % 2 == 1 -> solvable
    return ((inv + rowFromBottom) % 2) == 1;
}

// -------------------- A* (secuencial) -------------------------------------
struct Node {
    int f;
    int g;
    string state;
};
// Comparador: queremos un min-heap por f; si f igual, preferir mayor g (tiebreak)
struct NodeCompare {
    bool operator()(const Node& a, const Node& b) const {
        if (a.f != b.f) return a.f > b.f;     // menor f -> mayor prioridad
        return a.g < b.g;                     // si f igual, mayor g -> mayor prioridad
    }
};

// Devuelve costo (número de pasos) o -1 si no solucionable / no encontrado.
// Si path_out != nullptr, reconstruye la secuencia de acciones (strings).
static int astar(const string& start, int heur_id, vector<string>* path_out) {
    if (start == GOAL) {
        if (path_out) path_out->clear();
        return 0;
    }
    if (!solvable(start)) return -1;

    // inicializar tablas
    priority_queue<Node, vector<Node>, NodeCompare> open;
    unordered_map<string,int> gcost;                            // mejor g conocido
    unordered_map<string, pair<string,string>> parent;         // hijo -> (padre, accion)

    gcost.reserve(200000);
    parent.reserve(200000);

    int h0 = (heur_id == 1 ? h1_misplaced(start) : h2_manhattan(start));
    open.push(Node{h0, 0, start});
    gcost[start] = 0;

    vector<pair<string,string>> neigh; neigh.reserve(4);

    while (!open.empty()) {
        Node cur = open.top(); open.pop();
        const string &u = cur.state;
        int g = cur.g;

        // Si tenemos registrado un g mejor para u, saltamos (este nodo está obsoleto)
        auto itg = gcost.find(u);
        if (itg != gcost.end() && g > itg->second) continue;

        // Meta alcanzada
        if (u == GOAL) {
            if (path_out) {
                // reconstruir ruta desde GOAL hasta start
                vector<string> rev;
                string x = u;
                while (x != start) {
                    auto pp = parent[x];
                    rev.push_back(pp.second);
                    x = pp.first;
                }
                reverse(rev.begin(), rev.end());
                *path_out = rev;
            }
            return g;
        }

        // expandir vecinos
        vecinos(u, neigh);
        for (auto &p : neigh) {
            const string &v = p.first;
            const string &act = p.second;
            int ng = g + 1;
            auto it = gcost.find(v);
            if (it == gcost.end() || ng < it->second) {
                gcost[v] = ng;
                int h = (heur_id == 1 ? h1_misplaced(v) : h2_manhattan(v));
                open.push(Node{ng + h, ng, v});
                if (path_out) parent[v] = make_pair(u, act);
            }
        }
    }
    return -1;
}

// -------------------- BFS simple (para la opción 4) ------------------------
void bfs(const string& start) {
    if (start == GOAL) {
        cout << "El puzzle ya está resuelto.\n";
        cout << "Número mínimo de movimientos: 0\n";
        return;
    }
    if (!solvable(start)) {
        cout << "No es solucionable (paridad incompatible).\n";
        return;
    }

    queue<pair<string,int>> q;
    unordered_set<string> visited;
    q.push({start, 0});
    visited.insert(start);

    while (!q.empty()) {
        auto cur = q.front(); q.pop();
        string s = cur.first;
        int d = cur.second;
        if (s == GOAL) {
            cout << "Número mínimo de movimientos: " << d << "\n";
            return;
        }
        auto nb = obtenerVecinos(s);
        for (auto &p : nb) {
            const string &ns = p.first;
            if (!visited.count(ns)) {
                visited.insert(ns);
                q.push({ns, d + 1});
            }
        }
    }
    cout << "No se encontró solución (bfs terminó).\n";
}

// -------------------- interfaz principal -----------------------------------
int main() {
    // Inicializar tabla de posiciones objetivo (NECESARIO antes de usar h2)
    init_goal_pos();

    cout << "Seleccione la tarea a ejecutar:\n";
    cout << "  4 -> BFS costo\n";
    cout << "  5 -> A* h1 (costo)\n";
    cout << "  6 -> A* h2 (costo)\n";
    cout << "  7 -> A* h2 (acciones)\n";
    cout << "Opción: ";
    int tarea;
    cin >> tarea;
    if (cin.fail() || tarea < 4 || tarea > 7) {
        cerr << "Tarea no válida. Debe ser 4,5,6 o 7.\n";
        return 1;
    }

    cout << "Ingrese el estado inicial (16 caracteres, use '#' para el hueco):\n";
    string start;
    cin >> start;
    if (start.size() != 16) {
        cerr << "Error: el estado debe tener exactamente 16 caracteres.\n";
        return 1;
    }

    if (tarea == 4) {
        // BFS por niveles (devuelve costo mínimo)
        bfs(start);
    } else if (tarea == 5) {
        int cost = astar(start, 1, nullptr);
        if (cost < 0) cout << "UNSOLVABLE\n"; else cout << "Costo: " << cost << "\n";
    } else if (tarea == 6) {
        int cost = astar(start, 2, nullptr);
        if (cost < 0) cout << "UNSOLVABLE\n"; else cout << "Costo: " << cost << "\n";
    } else { // tarea == 7
        vector<string> path;
        int cost = astar(start, 2, &path);
        if (cost < 0) {
            cout << "UNSOLVABLE\n";
            return 0;
        }
        cout << "Costo: " << cost << "\n";
        cout << "Secuencia de acciones:\n";
        for (auto &a : path) cout << a << "\n";
    }

    return 0;
}
