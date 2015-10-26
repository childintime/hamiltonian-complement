#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stack>

// graf
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
void hc(Graph graph);
void hc_stack();

void print_graph(Graph graph);



// struktura s resenim
struct Solution {
    int n;
    int edges;
    int** graph;
};


int best_solution;
struct Solution solution;

void print_solution(struct Solution);

std::stack<Graph> graph_stack;

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

    //int graph[n][n];

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
//      char *p = buffer;

        // na kazdy bude n cisel
        for (int v = 0; v < n; v++)
        {
            // postupne nacte radek do matice grafu, strtol si posouva pointer samo!

            graph.graph[i][v] = buffer[v] - '0';


        }
    }
    // struktura pro reseni
    //best_solution = n;
    //struct Solution solution;
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
    else {
        

        graph_stack.push(graph);

        //while()

      //  printf("Graf neni hamiltonovsky.\n");
        hc_stack();
        //hc(graph);
        print_solution(solution);
        

    }
    


    return 0;
}

void hc_stack() {
    while(graph_stack.size() > 0) {
        //printf("size: %lu\n", graph_stack.size());
        Graph graph = graph_stack.top();
        //printf("pop\n");
        graph_stack.pop();
       //printf("%d, solution: %d\n", graph.edge_count, solution.edges);
        //graph_stack.pop();

        // test jestli uz graf je hamiltonovsky, pokud ano a pouzilo se mene hran nez ma dosavadni nejlepsi reseni tak prepsat reseni.
        if(hamilton_test(graph)) {
            if (graph.edge_count < solution.edges) {
                solution.edges = graph.edge_count;
                for (int n1 = 0; n1 < graph.n; n1++) {
                    for (int n2 = 0; n2 < graph.n; n2++) {
                        solution.graph[n1][n2] = graph.graph[n1][n2];
                    }
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

                            Graph new_graph;

                            new_graph.n = graph.n;
                            new_graph.offset_i = graph.offset_i;
                            new_graph.offset_j = graph.offset_j+1;
                            new_graph.edge_count = graph.edge_count+1;

                            new_graph.graph = (int**) malloc(graph.n*sizeof(int*));

                            
                            for (int i = 0; i < graph.n; i++) {
                                new_graph.graph[i] = (int*) malloc(graph.n*sizeof(int));
                                for (int v = 0; v < graph.n; v++) {
                                    new_graph.graph[i][v] = graph.graph[i][v];
                                }
                            }

                            new_graph.graph[i][j] = 1;
                            new_graph.graph[j][i] = 1;
                    
                            // dam na zasobnik ten novy graf
                            graph_stack.push(new_graph);
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

void hc(Graph graph) {
    
    // ma smysl hledat reseni jenom pokud muzeme najit lepsi reseni, nez uz mame ... tzn pokud uz jsme do grafu pridali vic hran nez ma zatim nalezene nejlepsi reseni, nema smysl ve vetvi pokracovat
    if (graph.edge_count < solution.edges) {
        // test jestli uz graf je hamiltonovsky, pokud ano a pouzilo se mene hran nez ma dosavadni nejlepsi reseni tak prepsat reseni.
        if(hamilton_test(graph)) {
            if (graph.edge_count < solution.edges) {
                solution.edges = graph.edge_count;
                for (int n1 = 0; n1 < graph.n; n1++) {
                    for (int n2 = 0; n2 < graph.n; n2++) {
                        solution.graph[n1][n2] = graph.graph[n1][n2];
                    }
                }
                            //printf("mam NEJLEPSI reseni, pocet hran:%d \n", edge_count+1);
            }
            else {
                            //printf("mam reseni, ale neni nejlepsi, pocet hran:%d \n", edge_count+1);    
            }
        }

        // az narazim na nespojenou dvojici uzlu, tak pridam hranu a rekurze ... + pokracovani bez pridani hrany    
        for (int i = graph.offset_i; i < graph.n; i++) {
            for (int j = graph.offset_j; j < graph.n; j++) {
            //printf("i:%d j:%d\n", i, j);
                if (graph.graph[i][j] == 0) {

                    Graph new_graph;
                    
                    new_graph.n = graph.n;
                    new_graph.offset_i = graph.offset_i;
                    new_graph.offset_j = graph.offset_j+1;
                    new_graph.edge_count = graph.edge_count+1;

                    new_graph.graph = (int**) malloc(graph.n*sizeof(int*));

                    // budu cist n radek
                    for (int i = 0; i < graph.n; i++) {
                        new_graph.graph[i] = (int*) malloc(graph.n*sizeof(int));
                        // na kazdy bude n cisel
                        for (int v = 0; v < graph.n; v++) {
                            // postupne nacte radek do matice grafu, strtol si posouva pointer samo!
                            new_graph.graph[i][v] = graph.graph[i][v];
                        }
                    }

                    new_graph.graph[i][j] = 1;
                    new_graph.graph[j][i] = 1;
                    //printf("i:%d j:%d\n", i, j);
                    // rekurze
                    hc(new_graph);

//                    graph[i][j] = 0;
//                  graph[j][i] = 0;
                }
            }
            graph.offset_j = i+2;
        }
    }
    else {
        
    }
    for(int i = 0; i < graph.n; i++) {
            free(graph.graph[i]);
        }
        free(graph.graph);

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

bool hamilton(Graph graph, int path[], int position, int used[])
{
    
    // pokud uz je cesta dlouha jako je pocet vrcholu a existuje prechod z posledniho bodu do prvniho tak konec, mame kruznici -- jinak jsme nasli cesty delky n, ale neni to kruznice.
    if (position == graph.n) {
        
        if (graph.graph[path[0]][path[position-1]] == 1) {
            //print_hamilton_cycle(n, path);    
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
    for (int i = 0; i < graph.n; i++) {
        for (int v = 0; v < graph.n; v++) {
            printf("%d", graph.graph[i][v]);
        }
        printf("\n");
    }
}
/* stara verze
void print_hamilton_cycle(int n, int path[n]) {
    for (int i = 0; i < n; i++) {
            printf("%d ", path[i]);
    }

    printf("%d \n", path[0]);
}
*/
