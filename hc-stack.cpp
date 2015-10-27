#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stack>

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
void print_hamilton_cycle(int n, int path[n]);

// hlavni funkce, ktera naplni strukturu solution vyslednym grafem
void hc_stack();

// tisk grafu
void print_graph(Graph graph);



// struktura s resenim
struct Solution {
    int n;
    int edges;
    int** graph;
};

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
        graph_stack.push(graph);

        //  printf("Graf neni hamiltonovsky.\n");
        hc_stack();
        print_solution(solution);
    }
    
    return 0;
}

void hc_stack() {
    // dokud je neco v zasobniku tak hledani bezi
    while(graph_stack.size() > 0) {
        Graph graph = graph_stack.top();
        // smazani vrcholu
        graph_stack.pop();
       
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
                            // kopie grafu, pri pridani na zasobnik
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

                            // pridani hrany
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
    for (int i = 0; i < graph.n; i++) {
        for (int v = 0; v < graph.n; v++) {
            printf("%d", graph.graph[i][v]);
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

// hamiltonovska kruznice

void print_hamilton_cycle(int n, int path[n]) {
    for (int i = 0; i < n; i++) {
            printf("%d ", path[i]);
    }

    printf("%d \n", path[0]);
}

