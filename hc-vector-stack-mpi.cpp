#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <vector>
#include "mpi.h"

#define MSG_DATA 1
#define MSG_WORK_REQUEST 2
#define MSG_WORK_DATA 3
#define MSG_WORK_REQUEST_DENY 4
#define MSG_TOKEN 5
#define MSG_SOLUTION 6
#define MSG_STOP 7


#define WHITE 0
#define BLACK 1

#define WORK_REQUEST_PROBE_TRESHOLD 100
//#define WORK_REQUEST_PROBE_TRESHOLD 100



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

// pocet hran
int n;

//
int stop_work = 0;

// barva procesu
int process_color = WHITE;
// pesek
int token = 0;
int token_color = WHITE;

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
    //int n = 0;
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
            	token = 1;

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
                    print_graph(graph_to_send);
                    printf("Posilam graf do i: %d.\n", i);

                    MPI_Send(data, n*n+4, MPI_INT, i, tag, MPI_COMM_WORLD);
                }
                print_graph(graph_vector_stack.back());
        

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

        int flag;
        int msgtest = 0;
        
        // boolean jestli uz jsem zadal o praci
        int request_sent = 0;

        // velikost prichozich dat
        int message_size = 0;
        
        // dokud neni priznak na konec prace a nemam dolni mez reseni tak pocitam

        int cycles = 0;
        int rank_to_request = my_rank;
            
        while(!stop_work && solution.edges > 1) {
            /*
            if (cycles == WORK_REQUEST_PROBE_TRESHOLD) {
                MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
                //printf("%d\n", flag);
                if(flag) {
                    printf("nekdo na poslal zasssspravu");
                    switch (status.MPI_TAG) {
                        case MSG_SOLUTION:
                        printf("poslal reseni");
                        MPI_Recv (&msgtest, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                        break;
                        case MSG_WORK_REQUEST:
                        printf("poslal data");
                        MPI_Recv (&msgtest, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                        break;
                    }
                }
            }
            else {
                cycles++;
            }
            */
            
            int deny = 0;
            // dosla prace?
            if (graph_vector_stack.size() == 0) {
                // prace dosla -> process je idle
                // kdyz jsem 0 a jsem white tak poslu peska dal - edux navod 1. bod
                if(my_rank == 0) {
                	process_color = WHITE;
                	token_color = WHITE;
                	// poslu peska dalsimu procesu
                	MPI_Send(&token_color, 1, MPI_INT, (my_rank + 1) % number_of_processes, MSG_TOKEN, MPI_COMM_WORLD);
                	// uz nemam peska
                	token = 0;
                }
                // edux navod bod 3. druha cast
                if(token) {
                	process_color == WHITE;
                	// poslu peska dal
                	MPI_Send(&token_color, 1, MPI_INT, (my_rank + 1) % number_of_processes, MSG_TOKEN, MPI_COMM_WORLD);
                    token = 0;

                }
                process_color = WHITE;

                if (request_sent == 0) {
                    rank_to_request = (rank_to_request + 1) % number_of_processes;
                    if (rank_to_request == my_rank) {
                        rank_to_request = (rank_to_request + 1) % number_of_processes;    
                    }

                    printf("jsem proces %d a chci poslat zadost o praci procesu %d \n", my_rank, rank_to_request);
                    int a = 5;
                    MPI_Send(&a, 1, MPI_INT, rank_to_request, MSG_WORK_REQUEST, MPI_COMM_WORLD);
                
                    request_sent = 1;
                
                    
                    //MPI_Send(&a, 1, MPI_INT, 1, 1, MPI_COMM_WORLD);
                
                }
                else {
                    printf("Proces %d cekam na zpravu ", my_rank);
                    //MPI_Status s2;

                    
                    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                    printf("probe prosel");
                    MPI_Get_count(&status, MPI_INT, &message_size);
                    
                    printf(" zprava je od %d, tag: %d, ", status.MPI_SOURCE, status.MPI_TAG);

                    switch (status.MPI_TAG) {
                    	// edux 3. bod prvni cast
                    	case MSG_TOKEN:
                    		MPI_Recv (&msgtest, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                    		token = 1;
                    		if(my_rank == 0) {
                    			if(msgtest == WHITE) {
                    				printf("muzu koncit vypocet!!!!!\n");
                    				for(int process_i = 1; process_i < number_of_processes; process_i++) {
                    					MPI_Send(&msgtest, 1, MPI_INT, process_i, MSG_STOP, MPI_COMM_WORLD);	
                    				}
                    				stop_work = 1;
                    			}
                    			else {
                    				
                    				token_color = WHITE;
                					// poslu peska dalsimu procesu
                					MPI_Send(&token_color, 1, MPI_INT, (my_rank + 1) % number_of_processes, MSG_TOKEN, MPI_COMM_WORLD);
                                    token = 0;

                    			}
                    		}

                    		if(process_color == BLACK) {
                    			token_color == BLACK;
                    		}
                    		break;
                    	case MSG_STOP:
                    		MPI_Recv (&msgtest, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                    		printf("prislo, ze muzu skoncit!!!\n");
                    		stop_work = 1;
                    		break;
						case MSG_SOLUTION:
                            printf("misto dat prislo reseni");
                            MPI_Recv (&msgtest, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                            break;
                        case MSG_WORK_REQUEST:
                            
                            MPI_Recv (&msgtest, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                            printf("misto dat prisla zadost o prace, nemam odmitam");
                            MPI_Send(&msgtest, 1, MPI_INT, status.MPI_SOURCE, MSG_WORK_REQUEST_DENY, MPI_COMM_WORLD);
                            break;
                        
                        case MSG_WORK_REQUEST_DENY:
                            printf("misto dat prislo odmitnuti");
                            MPI_Recv (&msgtest, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                            deny = 1;
                            request_sent = 0;
                            break;
                        case MSG_WORK_DATA:
                            request_sent = 0;
                            deny = 0;
                            printf("prisly data, pocet grafu: %d", message_size/(n*n+4));
                            //int data[(message_size)/]
                            int number_of_graphs = message_size/(n*n+4);
                            int new_work[message_size];
                            int offset = n*n+4;
                            MPI_Recv (&new_work, message_size, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                           /* for (int test = 0; test < 100; test++) {
                                        printf("msg %d\n", new_work[test]);
                            }
*/
                            for (int graph_i = 0; graph_i < number_of_graphs; graph_i++) {
                                Graph received_graph;
                                received_graph.n = new_work[graph_i*offset];
                                received_graph.offset_i = new_work[graph_i*offset+1];
                                received_graph.offset_j = new_work[graph_i*offset+2];
                                received_graph.edge_count = new_work[graph_i*offset+3];

                                received_graph.graph = (int**) malloc(received_graph.n*sizeof(int*));

                                for (int g_i = 0; g_i < n; g_i++) {
                                    received_graph.graph[g_i] = (int*) malloc(received_graph.n*sizeof(int));
                                    for (int g_j = 0; g_j < n; g_j++) {
                                        //printf("a %d %d\n", g_i, received_graph.n);
                                        received_graph.graph[g_i][g_j] = new_work[4+g_i*n+g_j+graph_i*offset];
                                    }     
                                }
                                graph_vector_stack.push_back(received_graph);
                                //printf("graf %d\n", graph_i);
                               // print_graph(received_graph);
                            }



                            

                    }
                    printf("velikost zasobniku: %d, deny: %d\n", graph_vector_stack.size(), deny);
                    
                    //break;
                    

                    //if (deny == 1) {
                      //  break;
                    //}
                    
                    //break;
                }
                //break;
            }
            else {
                printf("proces %d jde pocitat \n", my_rank);
                hc_stack();
                printf("proces %d vyskocil ze zasobnik, velikost zasobniku: %d, zatim testu: %d, jeho nejlepsi reseni:%d\n", my_rank, graph_vector_stack.size(),hamilton_test_count, solution.edges);
            } 
        }        
        printf("proces %d vyskocil z while \n", my_rank);
        MPI_Barrier(MPI_COMM_WORLD);
        double t2 = MPI_Wtime(); /* koncovy cas */

        //printf("proces %d je hotov, pocet pridanych hran: %d, vysledny graf:\n", my_rank, solution.edges);
        //print_solution(solution);
        
        printf("Spotrebovany cas procesu %d je %fs, pocet hamilton testu: %d\n", my_rank, t2 - t1, hamilton_test_count);
        

        /* shut down MPI */
        MPI_Finalize();

        return 0;

        print_solution(solution);
    }
    
    return 0;
}

void hc_stack() {
    int cycles = 0;
    int flag;
    int msgtest = 0;
    // dokud je neco v zasobniku tak hledani bezi
    while(graph_vector_stack.size() > 0) {
        if(cycles == WORK_REQUEST_PROBE_TRESHOLD) {
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
                //printf("%d\n", flag);
                if(flag) {
                    printf("Proces %d dostal zpravu od procesu %d  - ", my_rank, status.MPI_SOURCE);
                    switch (status.MPI_TAG) {
                    	case MSG_TOKEN:
                    		printf("prisel nam pesek");
                    		MPI_Recv (&msgtest, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                    		printf("- jeho barva je %d \n", msgtest);
							token = 1;
                    		token_color = msgtest;
                    		// edux navod 3. bod
                    		if(process_color == BLACK) {
                    			token_color == BLACK;
                    		}

                    		break;
						case MSG_STOP:
                    		MPI_Recv (&msgtest, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                            printf("prislo, ze mam skoncit, takze stopwork=1!");
                    		stop_work = 1;
                    		break;
                    	case MSG_WORK_REQUEST:
                    	printf("poslal nam zadost o data! Mam jich: %d", graph_vector_stack.size());
                    	MPI_Recv (&msgtest, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                        // tak mu neco poslem
                    	int a = 5;
                    	int process = 1;


                    	if (graph_vector_stack.size() > 2) {
                            Graph bottom = graph_vector_stack[0];
                            int bottom_edges = bottom.edge_count;
                            int test = bottom_edges;
                            int bottom_i = 0;
                            
                            while(test = bottom_edges && bottom_i < graph_vector_stack.size()) {
                                Graph t2 = graph_vector_stack[bottom_i];
                                test = t2.edge_count;
                                bottom_i++;
                            }
                            
                            printf("mam tu %d grafu na vrstve dna s %d hranama. takze mu jich zkusim poslat: %d", bottom_i, bottom_edges, bottom_i/2);


                    		int number_of_graphs = bottom_i/2;
                    		int offset = n*n+4;
                    		int data[number_of_graphs*offset];

                    		for (int graph_i = 0; graph_i < number_of_graphs; graph_i++) {
                    			Graph graph_to_send = graph_vector_stack.front();
                    			graph_vector_stack.erase(graph_vector_stack.begin());

                    			data[graph_i*offset] = graph_to_send.n;

                    			data[1+graph_i*offset] = graph_to_send.offset_i;
                    			data[2+graph_i*offset] = graph_to_send.offset_j;
                    			data[3+graph_i*offset] = graph_to_send.edge_count;

                    			for (int g_i = 0; g_i < n; g_i++) {
                    				for (int g_j = 0; g_j < n; g_j++) {
                            // prevedeni matice do jednorozmerneho pole
                    					data[4+g_i*n+g_j+graph_i*offset] = graph_to_send.graph[g_i][g_j];
                                //data[0][4+(g_i-1)*n+g_j] = 0;//graph_to_send.graph[g_i][g_j];
                    				}

                    			}
                                //print_graph(graph_to_send);
                    		}
                            // posilam data
                    		MPI_Send(data, number_of_graphs*offset, MPI_INT, status.MPI_SOURCE, MSG_WORK_DATA, MPI_COMM_WORLD);     
                            printf("tak jsem mu je poslal! ted jich mam: %d\n", graph_vector_stack.size());

                            // poslal jsem data, tak jeste musim vyresit pesky (bod 2. z navodu na eduxu)
                    		if(my_rank > status.MPI_SOURCE) {
                    			process_color = BLACK;
                    		}
                    	} 
                    	else {
                    		printf("uz toho mam malo, poslu mu odmitnuti!\n");
                    		MPI_Send(&msgtest, 1, MPI_INT, status.MPI_SOURCE, MSG_WORK_REQUEST_DENY, MPI_COMM_WORLD);
                    	}
                        //tenhe break tu nebyl
                    	break;

                    }
                }
                cycles = 0;
        }
        else {
            cycles++;
        }

        if(stop_work == 1) {
            printf("Proces %d: mam stopwork=1, takze break!\n", my_rank);
        	break;
        }

        //printf("vector size%d\n", graph_vector_stack.size());
        Graph graph = graph_vector_stack.back();

        if(my_rank == 1) {
        //print_graph(graph);
        }
        // smazani vrcholu
        graph_vector_stack.pop_back();
       
        hamilton_test_count++;

        if(hamilton_test_count % 10000 == 0) {
        	printf("%d %d %d\n",my_rank, hamilton_test_count, solution.edges );
        }
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
                    printf("Proces %d: mam dolni mez, koncim hledani ", my_rank);
                    for(int process_i = 0; process_i < number_of_processes; process_i++) {
                        if(process_i != my_rank) {
                            printf(", posilam stop procesu %d\n", process_i);
                            MPI_Send(&msgtest, 1, MPI_INT, process_i, MSG_STOP, MPI_COMM_WORLD);        
                        }
                    }
                    return;

                }
                printf("Proces %d: mam NEJLEPSI reseni, pocet hran:%d \n", my_rank, solution.edges);
            }
            else {
                printf("Proces %d: mam reseni, ale neni nejlepsi, pocet hran:%d \n", my_rank, graph.edge_count);    
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
    printf("proces: %d, offset: %d, %d\n", my_rank, graph.offset_i, graph.offset_j);
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
