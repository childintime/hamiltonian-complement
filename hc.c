#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* test jestli graf je hamiltonovsky */
bool hamilton(int n, int graph[][n], int path[n], int position, int used[n]);

/* hamilton test */
bool hamilton_test(int n, int graph[][n]);

/* tisk cesty */
void print_path(int n, int path[n]);

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

    printf("Pocet vrcholu: %d\n", n);

    int graph[n][n];

    // budu cist n radek
    for (int i = 0; i < n; i++) {
    	
    	fgets(buffer, sizeof(buffer), graphFile);
		char *p = buffer;

		// na kazdy bude n cisel
    	for (int v = 0; v < n; v++)
    	{
    		// postupne nacte radek do matice grafu, strtol si posouva pointer samo!
    		graph[i][v] = strtol(p, &p, 10);
    	}
	}


	// ------------- mame matici a jdem overit jestli je hamiltonovska

    
    if(hamilton_test(n, graph)) {
        printf("Graf je hamiltonovsky s vyse uvedenou cestou.\n");
    }
    else {
        printf("Graf neni hamiltonovsky.\n");
    }

    return 0;
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
    // vytisteni matice grafu pro overeni
    /*
    for (int i = 0; i < n; i++) {
        for (int v = 0; v < n; v++) {
            printf("%4d ", graph[i][v]);
        }
        printf("\n");
    }
    */
    

    // pokud uz je cesta dlouha jako je pocet vrcholu a existuje prechod z posledniho bodu do prvniho tak konec, mame kruznici -- jinak jsme nasli cesty delky n, ale neni to kruznice.
    if (position == n) {
        
        if (graph[path[0]][path[position-1]] == 1) {
            print_path(n, path);    
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
        }
    }
    
    return false; 
}

// tisk cesty
void print_path(int n, int path[n]) {
    for (int i = 0; i < n; i++) {
            printf("%d ", path[i]);
    }
    printf("\n");
}
