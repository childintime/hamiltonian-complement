#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


/* test zda je zadany graf hamiltonovsky*/
bool hamilton_test(int n, int graph[][n]);

/* rekurzivni funkce pro test */
bool hamilton(int n, int graph[][n], int path[n], int position, int used[n]);


/* vypise kruznici */
void print_hamilton_cycle(int n, int path[n]);

// hlavni funkce, ktera naplni strukturu solution vyslednym grafem
void hc(int n, int graph[][n], int offset_i, int offset_j, int edge_count);

void print_graph(int n, int graph[][n]);



// struktura s resenim
struct Solution {
    int n;
    int edges;
    int** graph;
};

int best_solution;
struct Solution solution;

void print_solution(struct Solution);


int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("usage: %s file_with_graph\n", argv[0]);
        return 1;
    }
    
    FILE *graphFile;
    graphFile = fopen(argv[1], "r");

    if (graphFile == NULL)
    {
        printf("Error reading file!\n");
        return 1;
    }

    char buffer[1024];
    fgets(buffer, sizeof(buffer), graphFile);
    int n = 0;
    n = atoi(buffer);

    //printf("Pocet vrcholu: %d\n", n);

    int graph[n][n];

    
    

    // budu cist n radek
    for (int i = 0; i < n; i++) {
        
        fgets(buffer, sizeof(buffer), graphFile);
//      char *p = buffer;

        // na kazdy bude n cisel
        for (int v = 0; v < n; v++)
        {
            // postupne nacte radek do matice grafu, strtol si posouva pointer samo!

            graph[i][v] = buffer[v] - '0';


        }
    }
    // struktura pro reseni
    //best_solution = n;
    //struct Solution solution;
    solution.n = n;
    solution.edges = n;
    solution.graph = malloc(n*sizeof(int*));
    for (int i = 0; i < n; i++) {
            solution.graph[i] = malloc(n*sizeof(int));
    }


    // ------------- mame matici a jdem overit jestli je hamiltonovska

    
    if(hamilton_test(n, graph)) {
        printf("Graf je hamiltonovsky.\n");
    }
    else {
        //printf("Graf neni hamiltonovsky.\n");
        
        hc(n, graph, 0, 1, 0);
        print_solution(solution);
        

    }


    return 0;
}

void hc(int n, int graph[][n], int offset_i, int offset_j, int edge_count) {

    if(hamilton_test(n, graph)) {

        if (edge_count < solution.edges) {
            solution.edges = edge_count;
            for (int n1 = 0; n1 < n; n1++) {
                for (int n2 = 0; n2 < n; n2++) {
                    solution.graph[n1][n2] = graph[n1][n2];
                }
            }
                            //printf("mam NEJLEPSI reseni, pocet hran:%d \n", edge_count+1);
        }
        else {
                            //printf("mam reseni, ale neni nejlepsi, pocet hran:%d \n", edge_count+1);    
        }
    }
    
    for (int i = offset_i; i < n; i++) {
        for (int j = offset_j; j < n; j++) {
            //printf("i:%d j:%d\n", i, j);
                if (graph[i][j] == 0) {
                    graph[i][j] = 1;
                    graph[j][i] = 1;
                    
                    hc(n, graph, i, j+1, edge_count+1);

                    graph[i][j] = 0;
                    graph[j][i] = 0;
                }
        }
        offset_j = i+2;
    }
}



bool hamilton_test(int n, int graph[][n]) {
    // cesta
    int path[n];
    
    // pole s pouzitymi vrcholy (0 = nepouzity, 1 = pouzity)
    int used[n];
    int position = 0;
    for (int i = 0; i < n; i++) {
        used[i] = 0;
    }

    //zacnem ve vrcholu 0:

    position = 1;
    path[0] = 0;
    used[0] = 1;

    if(hamilton(n, graph, path, position, used)) {
        return true;
    }
    else {
        return false;
    }

}


/* test jestli graf je hamiltonovsky */
bool hamilton(int n, int graph[][n], int path[n], int position, int used[n])
{
    
    // pokud uz je cesta dlouha jako je pocet vrcholu a existuje prechod z posledniho bodu do prvniho tak konec, mame kruznici -- jinak jsme nasli cesty delky n, ale neni to kruznice.
    if (position == n) {
        
        if (graph[path[0]][path[position-1]] == 1) {
            //print_hamilton_cycle(n, path);    
            return true;
        }
        else {
            return false;   
        }
    }  

    for(int i = 0; i < n; i++) {
        // pokud existuje prechod z posledniho vrcholu cesty do vrcholu i a jeste jsem vrchol i nepouzil, tak ho pridam do cesty a pustim dalsi iteraci rekurze
        if(graph[path[position-1]][i] == 1 && used[i] == 0) {
            path[position] = i;
            used[i] = 1;
            if(hamilton(n, graph, path, position+1, used)) {
                return true;
            }
            // backtracking
            else {
                path[position] = -1;
                used[i] = 0;
            }
        }
    }
    
    return false; 
}

// tisk hamiltonovsky kruznice
void print_hamilton_cycle(int n, int path[n]) {
    for (int i = 0; i < n; i++) {
            printf("%d ", path[i]);
    }

    printf("%d \n", path[0]);
}

// vytisteni matice grafu
void print_graph(int n, int graph[][n]) {
    for (int i = 0; i < n; i++) {
        for (int v = 0; v < n; v++) {
            printf("%d", graph[i][v]);
        }
        printf("\n");
    }
}

void print_solution(struct Solution solution) {
    printf("%d\n", solution.edges);
    for (int i = 0; i < solution.n; i++) {
        for (int v = 0; v < solution.n; v++) {
            printf("%d", solution.graph[i][v]);
        }
        printf("\n");
    }
}