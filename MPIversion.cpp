/*
===============================================================================
    DESCRIPCIÓN:
    Este programa implementa una versión distribuida del algoritmo BFS
    (Breadth-First Search) para resolver el rompecabezas 15-Puzzle utilizando
    la biblioteca MPI (Message Passing Interface).

    Cada proceso trabaja sobre una parte de la frontera actual (nivel del BFS)
    y genera los nuevos estados posibles ("vecinos"). El proceso 0 coordina la
    distribución de trabajo, la recolección de resultados y la verificación de
    estados repetidos mediante una estructura compartida `visited`.

    ESTRATEGIA:
    - Descomposición por datos (cada proceso recibe una porción del espacio de búsqueda)
    - Comunicación sincrónica mediante MPI_Send y MPI_Recv
    - Sincronización de estados con MPI_Bcast y MPI_Allreduce

    MÉTRICAS:
    - Número total de procesos
    - Profundidad de búsqueda
    - Tiempo total de ejecución
    - Nodos expandidos globalmente

    COMPILACIÓN:
        mpic++ MPIversion.cpp -o MPIversion -std=c++17

    EJECUCIÓN:
        mpirun -np 4 ./MPIversion
===============================================================================
*/

#include <mpi.h>
#include <bits/stdc++.h>
using namespace std;

// Estado objetivo del rompecabezas 15
static const string GOAL = "ABCDEFGHIJKLMNO#";

// -----------------------------------------------------------------------------
// FUNCIÓN: vecinos
// Dado un estado del puzzle, genera todos los estados posibles moviendo
// la ficha vacía (‘#’) hacia arriba, abajo, izquierda o derecha.
// -----------------------------------------------------------------------------
inline vector<string> vecinos(const string &s)
{
    int pos = s.find('#');
    int r = pos / 4, c = pos % 4;
    vector<string> nb;

    // Función auxiliar para hacer swap y registrar el nuevo estado
    auto swp = [&](int np)
    {
        string t = s;
        swap(t[pos], t[np]);
        nb.push_back(t);
    };

    if (c < 3) swp(pos + 1);
    if (c > 0) swp(pos - 1);
    if (r > 0) swp(pos - 4);
    if (r < 3) swp(pos + 4);

    return nb;
}

// -----------------------------------------------------------------------------
// FUNCIÓN PRINCIPAL
// -----------------------------------------------------------------------------
int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);  // Inicialización de MPI

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // Identificador del proceso
    MPI_Comm_size(MPI_COMM_WORLD, &size);  // Total de procesos

    double t0 = MPI_Wtime();  // Tiempo inicial

    // -------------------------------------------------------------------------
    // Entrada: solo el proceso 0 solicita la cadena inicial al usuario
    // -------------------------------------------------------------------------
    string input;
    if (rank == 0) {
        if (argc < 2) {
            cerr << "Uso: mpirun -np 4 ./MPIversion <estado_inicial>\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        input = argv[1];
    }

    // Broadcast de la cadena inicial a todos los procesos
    int len = input.size();
    MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
    input.resize(len);
    MPI_Bcast(input.data(), len, MPI_CHAR, 0, MPI_COMM_WORLD);

    // -------------------------------------------------------------------------
    // Inicialización de estructuras de búsqueda
    // -------------------------------------------------------------------------
    unordered_set<string> visited;
    visited.reserve(1 << 20);

    vector<string> frontier;
    if (rank == 0)
        frontier.push_back(input);

    visited.insert(input);

    // Variables de control
    int depth = 0;
    bool found = false;
    uint64_t expanded_local = 0;
    uint64_t expanded_global = 0;

    // -------------------------------------------------------------------------
    // Bucle principal del BFS
    // Cada iteración procesa un "nivel" completo del árbol de búsqueda.
    // -------------------------------------------------------------------------
    while (true)
    {
        int n = 0;
        if (rank == 0)
            n = frontier.size();

        // Difundir tamaño de la frontera actual
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

        if (n == 0 || found)
            break;

        // ---------------------------------------------------------------------
        // DISTRIBUCIÓN DE TRABAJO ENTRE PROCESOS
        // Cada proceso recibe un bloque de la frontera actual.
        // ---------------------------------------------------------------------
        int chunk = (n + size - 1) / size;
        int L = rank * chunk;
        int R = min(n, L + chunk);
        vector<string> my_jobs;

        if (rank == 0)
        {
            // Envío de bloques a los demás procesos
            for (int p = 1; p < size; p++)
            {
                int l = p * chunk, r = min(n, l + chunk);
                int cnt = r - l;
                MPI_Send(&cnt, 1, MPI_INT, p, 0, MPI_COMM_WORLD);
                for (int i = l; i < r; i++)
                    MPI_Send(frontier[i].data(), 16, MPI_CHAR, p, 0, MPI_COMM_WORLD);
            }
            my_jobs.insert(my_jobs.end(), frontier.begin() + L, frontier.begin() + R);
        }
        else
        {
            // Recepción del bloque asignado
            int cnt;
            MPI_Recv(&cnt, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int i = 0; i < cnt; i++)
            {
                string s(16, ' ');
                MPI_Recv(s.data(), 16, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                my_jobs.push_back(s);
            }
        }

        // ---------------------------------------------------------------------
        // EXPANSIÓN DE NODOS (cada proceso genera sus vecinos locales)
        // ---------------------------------------------------------------------
        vector<string> local_next;
        for (auto &s : my_jobs)
        {
            expanded_local++;
            for (auto &ns : vecinos(s))
            {
                if (ns == GOAL)
                    found = true;
                local_next.push_back(ns);
            }
        }

        // ---------------------------------------------------------------------
        // RECOLECCIÓN DE RESULTADOS EN EL PROCESO 0
        // ---------------------------------------------------------------------
        int local_size = local_next.size();
        vector<int> recv_sizes(size);
        MPI_Gather(&local_size, 1, MPI_INT, recv_sizes.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

        vector<string> next_layer;

        if (rank == 0)
        {
            // Calcular total de nuevos estados recibidos
            int total = accumulate(recv_sizes.begin(), recv_sizes.end(), 0);
            next_layer.reserve(total);

            // Insertar los estados generados por el proceso 0
            for (auto &s : local_next)
                next_layer.push_back(s);

            // Recibir los estados de los demás procesos
            for (int p = 1; p < size; p++)
            {
                for (int i = 0; i < recv_sizes[p]; i++)
                {
                    string s(16, ' ');
                    MPI_Recv(s.data(), 16, MPI_CHAR, p, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    next_layer.push_back(s);
                }
            }

            // Eliminar duplicados (solo en el proceso 0)
            vector<string> filtered;
            filtered.reserve(next_layer.size());
            for (auto &ns : next_layer)
                if (!visited.count(ns))
                {
                    visited.insert(ns);
                    filtered.push_back(ns);
                }

            // Actualizar la frontera para el siguiente nivel
            frontier.swap(filtered);
        }
        else
        {
            // Envío de los estados generados al proceso 0
            for (auto &s : local_next)
                MPI_Send(s.data(), 16, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        }

        // ---------------------------------------------------------------------
        // SINCRONIZACIÓN ENTRE PROCESOS
        // ---------------------------------------------------------------------
        MPI_Allreduce(MPI_IN_PLACE, &found, 1, MPI_C_BOOL, MPI_LOR, MPI_COMM_WORLD);
        MPI_Allreduce(&expanded_local, &expanded_global, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);

        depth++;
        if (found)
            break;
    }

    // -------------------------------------------------------------------------
    // RESULTADOS FINALES
    // -------------------------------------------------------------------------
    double t1 = MPI_Wtime();

    if (rank == 0)
    {
        cout << "\n[RESULTADOS MPI BFS]\n";
        cout << "Procesos: " << size << endl;
        cout << "Profundidad alcanzada: " << depth << endl;
        cout << "Tiempo total (segundos): " << (t1 - t0) << endl;
        cout << "Nodos expandidos globalmente: " << expanded_global << endl;
    }

    MPI_Finalize();
    return 0;
}
