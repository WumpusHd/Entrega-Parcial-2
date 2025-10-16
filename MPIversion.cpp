#include <mpi.h>
#include <bits/stdc++.h>
using namespace std;

static const string GOAL = "ABCDEFGHIJKLMNO#";

inline vector<string> vecinos(const string &s)
{
    int pos = s.find('#');
    int r = pos / 4, c = pos % 4;
    vector<string> nb;
    auto swp = [&](int np)
    { string t=s; swap(t[pos],t[np]); nb.push_back(t); };
    if (c < 3)
        swp(pos + 1);
    if (c > 0)
        swp(pos - 1);
    if (r > 0)
        swp(pos - 4);
    if (r < 3)
        swp(pos + 4);
    return nb;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double t0 = MPI_Wtime();

    string input;
    if (rank == 0)
    {
        cout << "Ingrese cadena de 16 caracteres (# como espacio): ";
        cin >> input;
    }

    // Enviar el input a todos
    int len = input.size();
    MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
    input.resize(len);
    MPI_Bcast(input.data(), len, MPI_CHAR, 0, MPI_COMM_WORLD);

    unordered_set<string> visited;
    visited.reserve(1 << 20);
    vector<string> frontier;
    if (rank == 0)
        frontier.push_back(input);
    visited.insert(input);

    int depth = 0;
    bool found = false;
    uint64_t expanded_local = 0;
    uint64_t expanded_global = 0;

    while (true)
    {
        // rank 0 distribuye cantidad de nodos
        int n = 0;
        if (rank == 0)
            n = frontier.size();
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

        if (n == 0 || found)
            break;

        // determinar bloque asignado a cada proceso
        int chunk = (n + size - 1) / size;
        int L = rank * chunk;
        int R = min(n, L + chunk);
        vector<string> my_jobs;
        if (rank == 0)
        {
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
            int cnt;
            MPI_Recv(&cnt, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int i = 0; i < cnt; i++)
            {
                string s(16, ' ');
                MPI_Recv(s.data(), 16, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                my_jobs.push_back(s);
            }
        }

        // cada proceso expande sus nodos
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

        // recolectar tamaños
        int local_size = local_next.size();
        vector<int> recv_sizes(size);
        MPI_Gather(&local_size, 1, MPI_INT, recv_sizes.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

        // rank 0 combina resultados
        vector<string> next_layer;
        if (rank == 0)
        {
            int total = accumulate(recv_sizes.begin(), recv_sizes.end(), 0);
            next_layer.reserve(total);
            for (auto &s : local_next)
                next_layer.push_back(s);
            for (int p = 1; p < size; p++)
            {
                for (int i = 0; i < recv_sizes[p]; i++)
                {
                    string s(16, ' ');
                    MPI_Recv(s.data(), 16, MPI_CHAR, p, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    next_layer.push_back(s);
                }
            }
            // eliminar duplicados
            vector<string> filtered;
            filtered.reserve(next_layer.size());
            for (auto &ns : next_layer)
                if (!visited.count(ns))
                {
                    visited.insert(ns);
                    filtered.push_back(ns);
                }
            frontier.swap(filtered);
        }
        else
        {
            // enviar strings al rank 0
            for (auto &s : local_next)
                MPI_Send(s.data(), 16, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        }

        MPI_Allreduce(MPI_IN_PLACE, &found, 1, MPI_C_BOOL, MPI_LOR, MPI_COMM_WORLD);
        MPI_Allreduce(&expanded_local, &expanded_global, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);
        depth++;
        if (found)
            break;
    }

    double t1 = MPI_Wtime();

    if (rank == 0)
    {
        cout << "\n[Resultado MPI BFS]" << endl;
        cout << "Procesos: " << size << endl;
        cout << "Profundidad: " << depth << endl;
        cout << "Tiempo (s): " << (t1 - t0) << endl;
        cout << "Nodos expandidos: " << expanded_global << endl;
    }

    MPI_Finalize();
    return 0;
}
