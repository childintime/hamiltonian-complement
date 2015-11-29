#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <vector>
#include "mpi.h"

// struktura grafu, vcetne pozice a poctu pridanych hran ... vlastne stav
typedef struct Graph {
    // pocet vrcholu
    int n;
    // pozice v grafu
    int offset_i;
    int offset_j;

    // pocet pridanych hran
    int edge_count;

    // matice incidenci
    int** graph;
} Graph;



/* test zda je zadany graf hamiltonovsky*/
bool hamilton_test(Graph graph);

/* rekurzivni funkce pro test */
bool hamilton(Graph graph, int path[], int position, int used[]);


/* vypise kruznici */
//void print_hamilton_cycle(int n, int path[n]);

// hlavni funkce, ktera naplni strukturu solution vyslednym grafem
void hc_stack();

// tisk grafu
void print_graph(Graph graph);

// iterace zasobniku na zacatku pro prvni rozdeleni prace
void parallel_stack_init();



// struktura s resenim
struct Solution {
    int n;
    int edges;
    int** graph;
};

struct Solution solution;

void print_solution(struct Solution);


// promenne pro MPI
MPI_Status status;
int tag, message = 1;
int p;
int my_rank;
int number_of_processes;

// zasobnik na grafy, pomoci vectoru
std::vector<Graph> graph_vector_stack;

int hamilton_test_count = 0;

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

    // graf ze zadani
    Graph graph;
    graph.n = n;
    graph.offset_i = 0;
    graph.offset_j = 1;
    graph.edge_count = 0;

    graph.graph = (int**) malloc(n*sizeof(int*));

    // budu cist n radek
    for (int i = 0; i < n; i++) {
        
        fgets(buffer, sizeof(buffer), graphFile);

        graph.graph[i] = (int*) malloc(n*sizeof(int));

        // na kazdy bude n cisel
        for (int v = 0; v < n; v++)
        {
            // postupne nacte radek do matice grafu, po znaku, - '0' to srovna.
            graph.graph[i][v] = buffer[v] - '0';
        }
    }
    
    // struktura pro reseni
    solution.n = n;
    solution.edges = n;
    solution.graph = (int**) malloc(n*sizeof(int*));
    for (int i = 0; i < n; i++) {
            solution.graph[i] = (int*) malloc(n*sizeof(int));
    }


    //print_graph(graph);

    // ------------- mame matici a jdem overit jestli je hamiltonovska

        
    if(hamilton_test(graph)) {
        printf("Graf je hamiltonovsky.\n");
    }
    // neni
    else {
        // na vrchol zasobniku dam zadani
        graph_vector_stack.push_back(graph);
        //graph_vector_stack.pop_back();

        //                  start MPI
        /* start up MPI */
        MPI_Init( &argc, &argv );

        /* find out process rank */
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

        /* find out number of processes */
        MPI_Comm_size(MPI_COMM_WORLD, &number_of_processes);
        
        /* merime cas */
        //MPI_Barrier (); /* cekam na spusteni vsech procesu */
        double t1 = MPI_Wtime(); /* pocatecni cas */

        if(number_of_processes > 1) {



            if(my_rank == 0) {
                printf("Jsem master, celkem spusteno %d procesu.\n", number_of_processes);
                parallel_stack_init();
                printf("naiterovan zasobnik, pocet prvku: %d\n", graph_vector_stack.size());

                if(graph_vector_stack.size() < number_of_processes) {
                    printf("tohle skoro nema cenu rozdelovat, kombinaci je fakt malo,  nebude se rozposilat, ukoncit ostatni vlakna (TODO)\n");
                }
                int data[n*n+4];
                int vector_size = graph_vector_stack.size();


                for(int i = 1; i < vector_size; i++) {

                    Graph graph_to_send = graph_vector_stack.back();
                    graph_vector_stack.pop_back();


                    data[0] = graph_to_send.n;
                    data[1] = graph_to_send.offset_i;
                    data[2] = graph_to_send.offset_j;
                    data[3] = graph_to_send.edge_count;
                    for (int g_i = 0; g_i < n; g_i++) {
                        for (int g_j = 0; g_j < n; g_j++) {
                        // prevedeni matice do jednorozmerneho pole
                            data[4+g_i*n+g_j] = graph_to_send.graph[g_i][g_j];
                        }     
                    }
                //    print_graph(graph_to_send);
                    printf("Posilam graf do i: %d.\n", i);
                    MPI_Send(data, n*n+4, MPI_INT, i, tag, MPI_COMM_WORLD);
                }
                //print_graph(graph_vector_stack.back());
        

            }
            else {
                int data[n*n+4];
                printf("Jsem slave, muj rank: %d.\n", my_rank);
                MPI_Recv (&data, n*n+4, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
               // printf("Slave %d: dostal jsem zpravu.\n", my_rank);
                Graph received_graph;
                received_graph.n = data[0];
                received_graph.offset_i = data[1];
                received_graph.offset_j = data[2];
                received_graph.edge_count = data[3];

                received_graph.graph = (int**) malloc(received_graph.n*sizeof(int*));

                for (int g_i = 0; g_i < n; g_i++) {
                    received_graph.graph[g_i] = (int*) malloc(received_graph.n*sizeof(int));
                    for (int g_j = 0; g_j < n; g_j++) {
                        received_graph.graph[g_i][g_j] = data[4+g_i*received_graph.n+g_j];
                    }     
                }
            //printf("startovaci graf procesu %d, offset: %d,%d\n", my_rank, received_graph.offset_i, received_graph.offset_j);
            //print_graph(received_graph);

                // smazu ten vychozi graf
                graph_vector_stack.pop_back();
                // dam tam ten co mi prisel
                graph_vector_stack.push_back(received_graph);
            }

        }

        
        //            printf("Posilam do i: %d.\n", i);
                    
        
        hc_stack();
        printf("proces %d je hotov, pocet pridanych hran: %d, vysledny graf:\n", my_rank, solution.edges);
        print_solution(solution);
        double t2 = MPI_Wtime(); /* koncovy cas */
        printf("Spotrebovany cas procesu %d je %fs, pocet hamilton testu: %d\n", my_rank, t2 - t1, hamilton_test_count);
        

        /* shut down MPI */
        MPI_Finalize();

        return 0;

        
        //  printf("Graf neni hamiltonovsky.\n");
        hc_stack();
        print_solution(solution);
    }
    
    return 0;
}

void hc_stack() {
    // dokud je neco v zasobniku tak hledani bezi
    while(graph_vector_stack.size() > 0) {
        //printf("vector size%d\n", graph_vector_stack.size());
        Graph graph = graph_vector_stack.back();

        if(my_rank == 1) {
        //print_graph(graph);
        }
        // smazani vrcholu
        graph_vector_stack.pop_back();
       
        hamilton_test_count++;
        // test jestli uz graf je hamiltonovsky, pokud ano a pouzilo se mene hran nez ma dosavadni nejlepsi reseni tak prepsat reseni.
        if(hamilton_test(graph)) {
            if (graph.edge_count < solution.edges) {
                solution.edges = graph.edge_count;
                for (int n1 = 0; n1 < graph.n; n1++) {
                    for (int n2 = 0; n2 < graph.n; n2++) {
                        solution.graph[n1][n2] = graph.graph[n1][n2];
                    }
                }

                if (solution.edges == 1) {
                    //printf("dolni mez, koncim hledani\n");
                    return;

                }
                //printf("mam NEJLEPSI reseni, pocet hran:%d \n", solution.edges);
            }
            else {
                //printf("mam reseni, ale neni nejlepsi, pocet hran:%d \n", graph.edge_count);    
            }
        }
        // ma cenu hledat dal jenom pokud neni hamiltonovsky
        else {
            // ma smysl hledat reseni jenom pokud muzeme najit lepsi reseni, nez uz mame ... tzn pokud uz jsme do grafu pridali vic hran nez ma zatim nalezene nejlepsi reseni, nema smysl ve vetvi pokracovat
            // -1 je tam protoze to k reseni vede jenom pokud pridam hranu
            if (graph.edge_count < (solution.edges-1)) {
                // az narazim na nespojenou dvojici uzlu, tak pridam hranu a rekurze ... + pokracovani bez pridani hrany    
                for (int i = graph.offset_i; i < graph.n; i++) {
                    for (int j = graph.offset_j; j < graph.n; j++) {
            
                        if (graph.graph[i][j] == 0) {
                            // kopie grafu, pri pridani na zasobnik
                            Graph new_graph;

                            new_graph.n = graph.n;
                            new_graph.offset_i = i;
                            new_graph.offset_j = j;
                            new_graph.edge_count = graph.edge_count+1;

                            new_graph.graph = (int**) malloc(graph.n*sizeof(int*));

                            for (int i2 = 0; i2 < graph.n; i2++) {
                                new_graph.graph[i2] = (int*) malloc(graph.n*sizeof(int));
                                for (int v = 0; v < graph.n; v++) {
                                    new_graph.graph[i2][v] = graph.graph[i2][v];
                                }
                            }

                            // pridani hrany
                            new_graph.graph[i][j] = 1;
                            new_graph.graph[j][i] = 1;
                    
                            // dam na zasobnik ten novy graf
                            graph_vector_stack.push_back(new_graph);
                        }
                    }
                    graph.offset_j = i+2;
                }
            }
            else {

            }
        }
    
    for(int i = 0; i < graph.n; i++) {
            free(graph.graph[i]);
        }
        free(graph.graph);
    }
}

// test jestli graf je hamiltonovsky
bool hamilton_test(Graph graph) {
    // cesta
    int path[graph.n];
    
    // pole s pouzitymi vrcholy (0 = nepouzity, 1 = pouzity)
    int used[graph.n];
    int position = 0;
    for (int i = 0; i < graph.n; i++) {
        used[i] = 0;
    }

    //zacnem ve vrcholu 0:

    position = 1;
    path[0] = 0;
    used[0] = 1;

    if(hamilton(graph, path, position, used)) {
        return true;
    }
    else {
        return false;
    }

}

void parallel_stack_init() {
    
    // promenna ktera indikuje jestli jeste zasobnik roste
    int iter = 1;
    while(iter == 1) {
        iter = 0;
        //printf("vector size%d\n", graph_vector_stack.size());
        Graph graph = graph_vector_stack.back();

        // smazani vrcholu
        graph_vector_stack.pop_back();

        // az narazim na nespojenou dvojici uzlu, tak pridam hranu a rekurze ... + pokracovani bez pridani hrany    
        for (int i = graph.offset_i; i < graph.n; i++) {
            for (int j = graph.offset_j; j < graph.n; j++) {

                if (graph.graph[i][j] == 0) {
                            // kopie grafu, pri pridani na zasobnik
                    Graph new_graph;

                    new_graph.n = graph.n;
                    new_graph.offset_i = i;
                    new_graph.offset_j = j;
                    new_graph.edge_count = graph.edge_count+1;

                    new_graph.graph = (int**) malloc(graph.n*sizeof(int*));

                    for (int i2 = 0; i2 < graph.n; i2++) {
                        new_graph.graph[i2] = (int*) malloc(graph.n*sizeof(int));
                        for (int v = 0; v < graph.n; v++) {
                            new_graph.graph[i2][v] = graph.graph[i2][v];
                        }
                    }

                            // pridani hrany
                    new_graph.graph[i][j] = 1;
                    new_graph.graph[j][i] = 1;
                    
                            // dam na zasobnik ten novy graf
                    graph_vector_stack.push_back(new_graph);
                    //printf("pushback\n");
                    //print_graph(new_graph);


                    iter = 1;
                    
                    if(graph_vector_stack.size() == (number_of_processes-1)) {
                        graph.offset_i = i;
                        graph.offset_j = graph.offset_j+1;
                        graph_vector_stack.push_back(graph);
                      
                        return;
                    }
                    
                }
            }
            graph.offset_j = i+2;
        }
        printf("tady");

        for(int i = 0; i < graph.n; i++) {
            free(graph.graph[i]);
        }
        free(graph.graph);
    }
}

// rekurzivne volana funkce
bool hamilton(Graph graph, int path[], int position, int used[])
{
    
    // pokud uz je cesta dlouha jako je pocet vrcholu a existuje prechod z posledniho bodu do prvniho tak konec, mame kruznici -- jinak jsme nasli cesty delky n, ale neni to kruznice.
    if (position == graph.n) {
        
        if (graph.graph[path[0]][path[position-1]] == 1) {
            //print_hamilton_cycle(n, path); 
            /* 
            for (int i = 0; i < n; i++) {
                printf("%d ", path[i]);
            }

            printf("%d \n", path[0]);
            */   
            return true;
        }
        else {
            return false;   
        }
    }  

    for(int i = 0; i < graph.n; i++) {
        // pokud existuje prechod z posledniho vrcholu cesty do vrcholu i a jeste jsem vrchol i nepouzil, tak ho pridam do cesty a pustim dalsi iteraci rekurze
        if(graph.graph[path[position-1]][i] == 1 && used[i] == 0) {
            path[position] = i;
            used[i] = 1;
            if(hamilton(graph, path, position+1, used)) {
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



void print_graph(Graph graph) {
    printf("offset: %d, %d\n", graph.offset_i, graph.offset_j);
    for (int i = 0; i < graph.n; i++) {
        for (int v = 0; v < graph.n; v++) {
            printf("%d", graph.graph[i][v]);
        }
        printf("\n");
    }
}


void print_solution(struct Solution solution) {
    for (int i = 0; i < solution.n; i++) {
        for (int v = 0; v < solution.n; v++) {
            printf("%d", solution.graph[i][v]);
        }
        printf("\n");
    }
}

// hamiltonovska kruznice
/*
void print_hamilton_cycle(int n, int path[n]) {
    for (int i = 0; i < n; i++) {
            printf("%d ", path[i]);
    }

    printf("%d \n", path[0]);
}
*/
