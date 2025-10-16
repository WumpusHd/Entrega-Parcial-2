#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <cmath>
#include <unordered_map>

using namespace std;

const string GOAL = "ABCDEFGHIJKLMNO#";
void ejecutarInstruccion(const string &cadena, const string &accion) {
    string resultado = cadena;
    int indice = cadena.find('#');

    if (indice == string::npos) {
        cerr << "Error: No se encontró el carácter '#' en la cadena." << endl;
        return;
    }

    bool movimientoValido = true;

    if (accion == "DERECHA") {
        if (indice % 4 != 3)
            swap(resultado[indice], resultado[indice + 1]);
        else
            movimientoValido = false;
    } 
    else if (accion == "IZQUIERDA") {
        if (indice % 4 != 0)
            swap(resultado[indice], resultado[indice - 1]);
        else
            movimientoValido = false;
    } 
    else if (accion == "ARRIBA") {
        if (indice >= 4)
            swap(resultado[indice], resultado[indice - 4]);
        else
            movimientoValido = false;
    } 
    else if (accion == "ABAJO") {
        if (indice < 12)
            swap(resultado[indice], resultado[indice + 4]);
        else
            movimientoValido = false;
    } 
    else {
        cout << "Comando no reconocido." << endl;
        return;
    }

    if (!movimientoValido) {
        cout << "Movimiento no permitido desde la posición actual." << endl;
        return;
    }

    cout << "\nNueva configuración de la matriz:\n";
    for (int fila = 0; fila < 4; ++fila) {
        for (int col = 0; col < 4; ++col) {
            cout << resultado[fila * 4 + col] << ' ';
        }
        cout << '\n';

    }
}

void mostrarAccionesDisponibles(const string &estado, const string &orden) {
    int indice = estado.find('#');
    vector<string> movimientosPosibles;

    if (indice == string::npos) {
        cerr << "Error: No se encontró el carácter '#' en la cadena." << endl;
        return;
    }

    int fila = indice / 4;
    int columna = indice % 4;

    if (columna < 3) movimientosPosibles.push_back("RIGHT");
    if (columna > 0) movimientosPosibles.push_back("LEFT");
    if (fila > 0)    movimientosPosibles.push_back("UP");
    if (fila < 3)    movimientosPosibles.push_back("DOWN");

    cout << "Desde la posición actual puedes mover: ";
    for (const auto &mov : movimientosPosibles) {
        cout << mov << " ";
    }
    cout << '\n';
}

vector<pair<string, string>> obtenerVecinos(const string& state) {
    int pos = state.find('#');
    vector<pair<string, string>> neighbors;

    // RIGHT
    if (pos % 4 < 3) {
        string new_state = state;
        swap(new_state[pos], new_state[pos + 1]);
        neighbors.push_back({new_state, "RIGHT"});
    }

    // LEFT
    if (pos % 4 > 0) {
        string new_state = state;
        swap(new_state[pos], new_state[pos - 1]);
        neighbors.push_back({new_state, "LEFT"});
    }

    // UP
    if (pos / 4 > 0) {
        string new_state = state;
        swap(new_state[pos], new_state[pos - 4]);
        neighbors.push_back({new_state, "UP"});
    }

    // DOWN
    if (pos / 4 < 3) {
        string new_state = state;
        swap(new_state[pos], new_state[pos + 4]);
        neighbors.push_back({new_state, "DOWN"});
    }

    return neighbors;
}


struct Move { int dr, dc; const char* name; };
static const Move MOVES[4] = {
    {-1, 0, "UP"},
    {+1, 0, "DOWN"},
    { 0,-1, "LEFT"},
    { 0,+1, "RIGHT"},
};

static inline void vecinos(const string& s, vector<pair<string,string> >& out) {
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
            out.push_back(make_pair(t, string(MOVES[k].name)));
        }
    }
}

void bfs(const string& start) {
    string objetivo = "ABCDEFGHIJKLMNO#";

    if (start == objetivo) {
        cout << "El puzzle ya esta resuelto." << endl;
    }else{
        queue<pair<string, int>> q;  // Estado, profundidad
        unordered_set<string> visited; // Conjunto para estados visitados

        q.push({start, 0});
        visited.insert(start);

        bool found = false;
        string target_state;
        int min_moves = -1;

        while (!q.empty() && !found) {
            auto front = q.front();
            q.pop();
            string current = front.first;
            int depth = front.second;

            if (current == objetivo) {
                found = true;
                target_state = current;
                min_moves = depth;
                found = true;
            }else{
                auto neighbors = obtenerVecinos(current);
                for (const auto& p : neighbors) {
                    string next_state = p.first;
                    string move = p.second;
                    if (visited.find(next_state) == visited.end()) {
                        visited.insert(next_state);
                        q.push({next_state, depth + 1});
                    }
                }
            }
            
        }
        if (!found) {
            cout << "No hay solucion" << endl;
            return;
        }
        cout << "Numero minimo de movimientos: " << min_moves << endl;  
    }
}

static inline int inversions(const string& s) {
    string t; t.reserve(16);
    for (char ch : s) if (ch != '#') t.push_back(ch);
    int inv = 0;
    for (int i = 0; i < (int)t.size(); ++i)
        for (int j = i + 1; j < (int)t.size(); ++j)
            if (t[i] > t[j]) ++inv;
    return inv;
}


struct Node {
    int f, g;
    string state;
    // priority_queue en C++ es max-heap; invertimos para tener min-heap
    bool operator<(const Node& other) const {
        if (f != other.f) return f > other.f;
        // tie-break por g mayor (más profundo) para favorecer expansiones más cercanas a la meta
        return g < other.g;
    }
};


// h1: fichas fuera de lugar (ignorar '#')
static inline int h1_misplaced(const string& s) {
    int c = 0;
    for (int i = 0; i < 16; ++i)
        if (s[i] != '#' && s[i] != GOAL[i]) ++c;
    return c;
}


static int goal_r[256], goal_c[256];
static void init_goal_pos() {
    for (int i = 0; i < 256; ++i) goal_r[i] = goal_c[i] = -1;
    for (int i = 0; i < 16; ++i) {
        char ch = GOAL[i];
        goal_r[(unsigned char)ch] = i / 4;
        goal_c[(unsigned char)ch] = i % 4;
    }
}

// h2: Manhattan (ignorar '#')
static inline int h2_manhattan(const string& s) {
    int sum = 0;
    for (int i = 0; i < 16; ++i) {
        char ch = s[i];
        if (ch == '#') continue;
        int r = i / 4, c = i % 4;
        int gr = goal_r[(unsigned char)ch], gc = goal_c[(unsigned char)ch];
        sum += abs(r - gr) + abs(c - gc);
    }
    return sum;
}

static inline bool solvable(const string& s) {
    int inv = inversions(s);
    int blank = (int)s.find('#');
    int rowFromBottom = 4 - (blank / 4); // 1..4 (1 = última fila)
    return ((inv + rowFromBottom) % 2) == 1;
}

// A*: heur_id = 1 -> h1, heur_id = 2 -> h2
// Si path_out != NULL, se reconstruyen acciones
static int astar(const string& start, int heur_id, vector<string>* path_out) {
    if (start == GOAL) {
        if (path_out) path_out->clear();
        return 0;
    }
    if (!solvable(start)) return -1;

    // Estructuras A*
    priority_queue<Node> open;
    unordered_map<string,int> gcost; // mejor g conocido
    unordered_map<string, pair<string,string> > parent; // estado -> (padre, acción)

    gcost.reserve(200000);
    parent.reserve(200000);

    int h0 = (heur_id == 1 ? h1_misplaced(start) : h2_manhattan(start));
    open.push(Node{h0, 0, start});
    gcost[start] = 0;

    vector<pair<string,string> > neigh; neigh.reserve(4);

    while (!open.empty()) {
        Node cur = open.top(); open.pop();
        const string& u = cur.state;
        int g = cur.g;

        // Podría estar obsoleto si ya conocemos un g mejor
        unordered_map<string,int>::iterator itg = gcost.find(u);
        if (itg == gcost.end() || g > itg->second) continue;

        if (u == GOAL) {
            if (path_out) {
                // reconstruir
                vector<string> rev;
                string x = u;
                while (x != start) {
                    pair<string,string> pp = parent[x];
                    rev.push_back(pp.second);
                    x = pp.first;
                }
                reverse(rev.begin(), rev.end());
                *path_out = rev;
            }
            return g;
        }

        vecinos(u, neigh);
        for (size_t i = 0; i < neigh.size(); ++i) {
            const string& v = neigh[i].first;
            const string& act = neigh[i].second;
            int ng = g + 1;
            if (!gcost.count(v) || ng < gcost[v]) {
                gcost[v] = ng;
                int h = (heur_id == 1 ? h1_misplaced(v) : h2_manhattan(v));
                open.push(Node{ng + h, ng, v});
                if (path_out) parent[v] = make_pair(u, act);
            }
        }
    }
    return -1;
}


int main(){
    
    int tarea;
    cout << "Seleccione la tarea a ejecutar:\n";
    cout << "  4 -> BFS costo\n";
    cout << "  5 -> A* h1 (costo)\n";
    cout << "  6 -> A* h2 (costo)\n";
    cout << "  7 -> A* h2 (acciones)\n";
    cout << "Opción: ";
    cin >> tarea;

    if (cin.fail() || tarea < 4 || tarea > 7) {
        cerr << "Tarea no válida. Debe ser 4, 5, 6 o 7.\n";
        return 1;
    }

    string start;
    cout << "Ingrese el estado inicial (16 caracteres): ";
    cin >> start;

    if (start.size() != 16) {
        cerr << "Error: el estado debe tener exactamente 16 caracteres.\n";
        return 1;
    }

    if (tarea == 4) {
        bfs(start);
    } 
    else if (tarea == 5) {
        int cost = astar(start, 1, nullptr);
        if (cost < 0) cout << "UNSOLVABLE\n";
        else cout << "Costo: " << cost << "\n";
    } 
    else if (tarea == 6) {
        int cost = astar(start, 2, nullptr);
        if (cost < 0) cout << "UNSOLVABLE\n";
        else cout << "Costo: " << cost << "\n";
    } 
    else if (tarea == 7) {
        vector<string> path;
        int cost = astar(start, 2, &path);
        if (cost < 0) {
            cout << "UNSOLVABLE\n";
            return 0;
        }

        // Si quieres, puedes mostrar también el costo
        // cout << "Costo: " << cost << "\n";
        cout << "Secuencia de acciones:\n";
        for (const auto& step : path) {
            cout << step << "\n";
        }
    }
    return 0;
}
