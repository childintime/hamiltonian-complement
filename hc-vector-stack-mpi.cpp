#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <vector>
#include "mpi.h"

/* tagy pro MPI zpravy */
#define MSG_DATA 1
#define MSG_WORK_REQUEST 2
#define MSG_WORK_DATA 3
#define MSG_WORK_REQUEST_DENY 4
#define MSG_TOKEN 5
#define MSG_SOLUTION 6
#define MSG_STOP 7

/* barvy pro procesy a peska */
#define WHITE 0
#define BLACK 1

#define PROBE_TRESHOLD 100
//#define WORK_REQUEST_PROBE_TRESHOLD 100

#define PRINT_MPI 1
#define PRINT_STATE 1
#define PRINT_INIT_GRAPHS 0

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

// zasobnik na grafy, pomoci vectoru
std::vector<Graph> graph_vector_stack;


/* test zda je zadany graf hamiltonovsky*/
bool hamilton_test(Graph graph);

/* rekurzivni funkce pro test */
bool hamilton(Graph graph, int path[], int position, int used[]);


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

// tisk reseni
void print_solution(struct Solution);


// promenne pro MPI
MPI_Status status;
int tag;
int flag;
int p;
int my_rank;
int number_of_processes;
int blank = 0;
// pocet hran
int n;

//
int stop_work = 0;

/* barva procesu */
int process_color = WHITE;
/* mam peska? */
int token = 0;
/* barva peska */
int token_color = WHITE;

/* pocet hamilton test */
int hamilton_test_count = 0;

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("usage: %s file_with_graph\n", argv[0]);
        return 1;
    }
    

    /*      NACITANI DAT        */
    FILE *graphFile;
    graphFile = fopen(argv[1], "r");

    if (graphFile == NULL)
    {
        printf("Error reading file!\n");
        return 1;
    }

    char buffer[1024];
    fgets(buffer, sizeof(buffer), graphFile);
    // pocet hran
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
    /* v graph je nacten graf ze zadani */
    
    // struktura pro reseni
    solution.n = n;
    solution.edges = n;
    solution.graph = (int**) malloc(n*sizeof(int*));
    for (int i = 0; i < n; i++) {
        solution.graph[i] = (int*) malloc(n*sizeof(int));
    }


    /* prvotni test, jestli je hraf hamiltonovsky */
    if(hamilton_test(graph)) {
        printf("Graf je hamiltonovsky.\n");
    }
    // neni
    else {
        /* na vrchol zasobniku dam zadani */
        graph_vector_stack.push_back(graph);
        
        /* start MPI */
        MPI_Init( &argc, &argv );

        /* zjisteni vlastniho rank */
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

        /* zjisteni poctu procesut */
        MPI_Comm_size(MPI_COMM_WORLD, &number_of_processes);
        
        /* merime cas */
        //MPI_Barrier (); /* cekam na spusteni vsech procesu */
        double t1 = MPI_Wtime(); /* pocatecni cas */

        /* pokud je jen jeden proces tak pokracuju jako by to byl sekvencni program */
        if(number_of_processes > 1) {

            /*  v tomto kroku master naiteruje zasobnik
                a rozposle ostatnim procesum jejich pocatecni grafy
            */
            if(my_rank == 0) {
                /* na zacatku ma peska master proces */
            	token = 1;

                #if PRINT_STATE
                printf("Jsem master, celkem spusteno %d procesu.\n", number_of_processes);
                #endif                
                
                /* naiteruju zasobnik (pro dany pocet procesu) */
                parallel_stack_init();

                #if PRINT_STATE                
                printf("naiterovan zasobnik, pocet prvku: %d\n", graph_vector_stack.size());
                if(graph_vector_stack.size() < number_of_processes) {
                    printf("tohle skoro nema cenu rozdelovat, kombinaci je fakt malo,  nebude se rozposilat, ukoncit ostatni vlakna (TODO)\n");
                }
                #endif
                
                /* pole pro posilani grafu */
                int data[n*n+4];
                int vector_size = graph_vector_stack.size();
                
                for(int i = 1; i < vector_size; i++) {

                    /* graf z vrcholu zasobniku prevedu do pole a poslu */
                    Graph graph_to_send = graph_vector_stack.back();
                    graph_vector_stack.pop_back();

                    data[0] = graph_to_send.n;
                    data[1] = graph_to_send.offset_i;
                    data[2] = graph_to_send.offset_j;
                    data[3] = graph_to_send.edge_count;
                    for (int g_i = 0; g_i < n; g_i++) {
                        for (int g_j = 0; g_j < n; g_j++) {
                            data[4+g_i*n+g_j] = graph_to_send.graph[g_i][g_j];
                        }     
                    }
                    
                    #if PRINT_INIT_GRAPHS     
                    printf("Proces 0: posilam nasledujici graf procesu %d.\n", i);               
                    print_graph(graph_to_send);
                    #endif                    

                    MPI_Send(data, n*n+4, MPI_INT, i, tag, MPI_COMM_WORLD);
                }
                
                #if PRINT_INIT_GRAPHS
                printf("Proces 0: nasledujici graf je muj.\n");                
                print_graph(graph_vector_stack.back());
                #endif                
            
            }
            else {
                /* pole pro prijimani grafu */
                int data[n*n+4];
                
                #if PRINT_STATE
                printf("Proces %d: jsem slave.\n", my_rank);
                #endif                
                
                MPI_Recv (&data, n*n+4, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                
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

                /* misto vychoziho grafu na zasobniku dam ten co prisel */
                graph_vector_stack.pop_back();
                graph_vector_stack.push_back(received_graph);
            }



            int message = 0;

            // boolean jestli uz jsem zadal o praci
            int request_sent = 0;

            // velikost prichozich dat
            int message_size = 0;

            // proces, ktereho se bude zadat o praci (bude se to posouvat)
            int rank_to_request = my_rank;
            
            // dokud neni priznak na konec prace a nemam dolni mez reseni tak pocitam
            while(!stop_work && solution.edges > 1) {

                // dosla prace?
                if (graph_vector_stack.size() == 0) {
                
                
                    #if PRINT_STATE                    
                    printf("Proces %d: process_color: %d, token: %d, token_color: %d \n", my_rank, process_color, token, token_color);
                    #endif                    
                    
                    // prace dosla -> process je idle
                    // kdyz jsem 0 a jsem white tak poslu peska dal - edux navod 1. bod
                    // mam peska?
                    if(token) {
                        if(my_rank == 0) {
                            token_color = WHITE;
                            // poslu bileho peska dal
                            MPI_Send(&token_color, 1, MPI_INT, (my_rank + 1) % number_of_processes, MSG_TOKEN, MPI_COMM_WORLD);    
                        }
                        else {
                            // poslu peska dal
                            MPI_Send(&token_color, 1, MPI_INT, (my_rank + 1) % number_of_processes, MSG_TOKEN, MPI_COMM_WORLD);
                        }
                        
                        #if PRINT_MPI                         
                        printf("Proces %d: idle a posila peska barvy %d procesu %d \n", my_rank, token_color, (my_rank + 1) % number_of_processes);
                        #endif                        
                        
                        // peska jsem poslal
                        token = 0;
                    }
                    // i kdyz nemam peska tak se stavam bilym
                    process_color = WHITE;

                    // pokud jsem uz neposilal zadost
                    if (request_sent == 0) {
                        rank_to_request = (rank_to_request + 1) % number_of_processes;
                        if (rank_to_request == my_rank) {
                            rank_to_request = (rank_to_request + 1) % number_of_processes;    
                        }
                        
                        #if PRINT_MPI
                        printf("Proces %d: posilam WORK_REQUEST procesu %d \n", my_rank, rank_to_request);
                        #endif
                        
                        // tak ji poslu
                        MPI_Send(&blank, 1, MPI_INT, rank_to_request, MSG_WORK_REQUEST, MPI_COMM_WORLD);
                        request_sent = 1;

                    }
                    // zadost jsem poslal, tak pockam na zpravu, bud to budou data, nebo pesek, nebo neco jinyho...
                    else {
                        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                        MPI_Get_count(&status, MPI_INT, &message_size);
                        
                        #if PRINT_MPI
                        printf("Proces %d: zprava od %d, tag: %d, ", my_rank, status.MPI_SOURCE, status.MPI_TAG);
                        #endif
                        
                        // podle tagu budu tridit zpravy
                        switch (status.MPI_TAG) {
                            // prisel token
                    	    case MSG_TOKEN:
                                MPI_Recv (&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                                token = 1;
                                token_color = message;

                                if(my_rank == 0) {
                                    // pokud prisel procesu 0 bily pesek, tak konec!
                                    if(token_color == WHITE) {
                                        
                                        #if PRINT_MPI
                                        printf("prisel bily pesek, takze konec celeho vypoctu!\n");
                                        #endif                                        
                                        
                                        for(int process_i = 1; process_i < number_of_processes; process_i++) {
                                            MPI_Send(&message, 1, MPI_INT, process_i, MSG_STOP, MPI_COMM_WORLD);	
                                        }
                                        stop_work = 1;
                                    }
                                    // poslu bily token dal, zadny konec
                                    else {
                                        token_color = WHITE;
                                        
                                        #if PRINT_MPI                                        
                					    printf("Proces %d: posilam peska barvy %d procesu %d \n", my_rank, token_color, 1);
                                        #endif                                        
                                        
                                        MPI_Send(&token_color, 1, MPI_INT, 1, MSG_TOKEN, MPI_COMM_WORLD);
                                        token = 0;
                                    }
                                }

                                // edux bod 3. prvni cast
                                if(process_color == BLACK) {
                                    token_color == BLACK;
                                }
                                break;

                            case MSG_STOP:

                                MPI_Recv (&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                                
                                #if PRINT_MPI
                                printf("prislo, ze muzu skoncit! mam v zasobniku: %d\n", graph_vector_stack.size());
                                #endif
                                
                                stop_work = 1;
                                break;
                 
                            case MSG_SOLUTION:
                                MPI_Recv (&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                                
                                #if PRINT_MPI                                
                                printf("prislo reseni \n");
                                #endif
                                
                                break;
                 
                            case MSG_WORK_REQUEST:
                                MPI_Recv (&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                                
                                #if PRINT_MPI                                
                                printf("prisla zadost o praci, sam nemam, odmitam \n");
                                #endif
                                
                                MPI_Send(&message, 1, MPI_INT, status.MPI_SOURCE, MSG_WORK_REQUEST_DENY, MPI_COMM_WORLD);
                                break;

                            case MSG_WORK_REQUEST_DENY:
                                MPI_Recv (&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                                
                                #if PRINT_MPI
                                printf("prislo odmitnuti zadosti o praci \n");
                                #endif
                                
                                request_sent = 0;
                                break;
                 
                            case MSG_WORK_DATA:
                                request_sent = 0;
                                
                                int number_of_graphs = message_size/(n*n+4);
                                int new_work[message_size];
                                int offset = n*n+4;
                                MPI_Recv (&new_work, message_size, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                                
                                #if PRINT_MPI
                                printf("prisly data, pocet grafu: %d\n", number_of_graphs);
                                #endif
                                
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
                                            received_graph.graph[g_i][g_j] = new_work[4+g_i*n+g_j+graph_i*offset];
                                        }     
                                    }
                                    
                                    graph_vector_stack.push_back(received_graph);
                                }
                                break;
                        }
                    }
                }
                // neco mam v zasobniku, muzu pocitat
                else {
                    #if PRINT_STATE                  
                    printf("Proces %d: jdu pocitat. \n", my_rank);
                    #endif

                    hc_stack();

                    #if PRINT_STATE
                    printf("Proces %d: vyskocil ze zasobniku, velikost zasobniku: %d, zatim testu: %d, jeho nejlepsi reseni:%d\n", my_rank, graph_vector_stack.size(),hamilton_test_count, solution.edges);
                    #endif
                } 
            }
            #if PRINT_STATE        
            printf("Proces %d: jsem pryc z vypoctu \n", my_rank);
            #endif

            // pockam az vyskoci vsichni
            MPI_Barrier(MPI_COMM_WORLD);

            int local_solution[2];
            local_solution[0] = solution.edges;
            local_solution[1] = my_rank;
            int global_solution[2];

            // vestavena redukce, allreduce zajisti, ze global_solution maji vsichni spravne, coz neni uplne treba, ale pak posle finalni reseni jenom ten nejlepsi
            MPI_Allreduce(local_solution, global_solution, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);


            if(my_rank == 0) {
                int final_solution[n*n+1];

                #if PRINT_STATE
                printf("Nejlepsi reseni ma proces %d a ma pocet hran: %d\n", global_solution[1], global_solution[0]);
                #endif
                
                // cekam az mi ho posle (pokud to nejsem ja (0), do ho ma)
                if(global_solution[1] != 0) {
                    MPI_Recv (&final_solution, n*n+1, MPI_INT, MPI_ANY_SOURCE, MSG_SOLUTION, MPI_COMM_WORLD, &status);
                    #if PRINT_MPI
                    printf("Proces 0: prislo reseni od %d, pocet hran: %d\n", status.MPI_SOURCE, final_solution[0]);
                    #endif

                    if(final_solution[0] < solution.edges) {
                        solution.edges = final_solution[0];
                        for (int solution_i = 0; solution_i < n; solution_i++) {
                            for (int solution_j = 0; solution_j < n; solution_j++) {
                                solution.graph[solution_i][solution_j] = final_solution[solution_i*n+solution_j+1];
                            }     
                        }
                    }
                }
                print_solution(solution);    
            }

            // pokud mam nejlepsi reseni, tak ho poslu 0
            if(global_solution[1] == my_rank && my_rank != 0) {

                int final_solution[n*n+1];
                #if PRINT_STATE
                printf("Proces %d: mam nejlepsi reseni.\n", global_solution[1]);
                #endif

                final_solution[0] = solution.edges;
                for (int solution_i = 0; solution_i < n; solution_i++) {
                    for (int solution_j = 0; solution_j < n; solution_j++) {
                        final_solution[solution_i*n+solution_j+1] = solution.graph[solution_i][solution_j];
                    }     
                }
                MPI_Send(&final_solution, n*n+1, MPI_INT, 0, MSG_SOLUTION, MPI_COMM_WORLD);    
            }

        }

        else {
            #if PRINT_STATE
            printf("Pocita jen jeden proces. \n");
            #endif

            hc_stack();
            print_solution(solution);
        }
        
        double t2 = MPI_Wtime(); /* koncovy cas */
        
        #if PRINT_STATE
        printf("Spotrebovany cas procesu %d je %fs, pocet hamilton testu: %d\n", my_rank, t2 - t1, hamilton_test_count);
        #endif

        /* shut down MPI */
        MPI_Finalize();
    }
    return 0;
}

void hc_stack() {
    int cycles = 0;
    int msgtest = 0;
    int message = 0;
    // dokud je neco v zasobniku tak hledani bezi
    while(graph_vector_stack.size() > 0) {
        // prichozi zpravy pri vypoctu nekontruluju porad, jednou za cas podle trehsholdu
        if(cycles == PROBE_TRESHOLD) {

            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
                
            if(flag) {
                #if PRINT_MPI
                printf("Proces %d: zprava od %d: ", my_rank, status.MPI_SOURCE);
                #endif

                switch (status.MPI_TAG) {
                   	case MSG_TOKEN:
                   		MPI_Recv (&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                        #if PRINT_MPI
                   		printf("prisel pesek barvy %d, mam %d v zasobniku \n", msgtest, graph_vector_stack.size());
                        #endif
							
                        // nejsme IDLE, peska nebudu posilat dal
                        token = 1;
                        token_color = message;
                        // edux navod bod 3. prvni cast
                        if(process_color == BLACK) {
                            token_color == BLACK;
                        }
                        break;

					case MSG_STOP:
                    	MPI_Recv (&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                        #if PRINT_MPI
                        printf("prislo ukonceni vypoctu, stop_work=1!");
                        #endif

                    	stop_work = 1;
                        break;

                    case MSG_WORK_REQUEST:
                        MPI_Recv (&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                            
                        Graph bottom = graph_vector_stack[0];
                        int bottom_edges = bottom.edge_count;
                        int test = bottom_edges;
                        int bottom_i = 0;
                            
                        while(test == bottom_edges && bottom_i < graph_vector_stack.size()) {
                            Graph t2 = graph_vector_stack[bottom_i];
                            test = t2.edge_count;
                            bottom_i++;
                        }
    
                        #if PRINT_MPI
                        printf("zadost o data! na vrstve dna mam %d grafu, ", bottom_i);
                        #endif

                        if(bottom_i > 1) {
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
                                    }
                                }
                            }

                            // posilam data
                            MPI_Send(data, number_of_graphs*offset, MPI_INT, status.MPI_SOURCE, MSG_WORK_DATA, MPI_COMM_WORLD);

                            #if PRINT_MPI
                            printf("posilam jich: %d\n", number_of_graphs);
                            #endif     

                            // poslal jsem data, tak jeste musim vyresit pesky (bod 2. z navodu na eduxu)
                            if(my_rank > status.MPI_SOURCE) {
                                process_color = BLACK;
                                
                                #if PRINT_STATE
                                printf("Proces %d: menim svoji barvu na BLACK!\n", my_rank);
                                #endif
                            }
                        }
                        else {
                            MPI_Send(&msgtest, 1, MPI_INT, status.MPI_SOURCE, MSG_WORK_REQUEST_DENY, MPI_COMM_WORLD);
                            
                            #if PRINT_STATE
                            printf("to je malo, posilam odmitnuti!\n");
                            #endif
                        }

                        break;
                }
            }
            cycles = 0;
        }
        else {
            cycles++;
        }

        if(stop_work == 1) {
            #if PRINT_STATE
            printf("Proces %d: mam stopwork=1, takze break!\n", my_rank);
            #endif

            // vyskoceni z while cyklu
        	break;
        }

        
        // ----- hlavni vypocet
        Graph graph = graph_vector_stack.back();
        // smazani vrcholu
        graph_vector_stack.pop_back();
       
        hamilton_test_count++;


        if(hamilton_test_count % 10000 == 0) {
            #if PRINT_STATE
        	printf("%d %d %d\n",my_rank, hamilton_test_count, solution.edges );
            #endif
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
                    #if PRINT_STATE
                    printf("Proces %d: mam dolni mez, koncim hledani ", my_rank);
                    #endif

                    for(int process_i = 0; process_i < number_of_processes; process_i++) {
                        // sobe posilat reseni nebudu, to zaroven osetri beh s jednim procesem
                        if(process_i != my_rank) {
                            #if PRINT_STATE
                            printf(", posilam stop procesu %d\n", process_i);
                            #endif

                            MPI_Send(&msgtest, 1, MPI_INT, process_i, MSG_STOP, MPI_COMM_WORLD);        
                        }
                    }
                    return;

                }
                #if PRINT_STATE
                printf("Proces %d: mam reseni, pocet hran:%d \n", my_rank, solution.edges);
                #endif
            }
            else {
                #if PRINT_STATE
                printf("Proces %d: mam reseni, ale neni nejlepsi, pocet hran:%d \n", my_rank, graph.edge_count);
                #endif
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
    printf("Proces %d: offset: %d, %d\n", my_rank, graph.offset_i, graph.offset_j);
    for (int i = 0; i < graph.n; i++) {
        for (int v = 0; v < graph.n; v++) {
            printf("%d", graph.graph[i][v]);
        }
        printf("\n");
    }
}

void print_solution(struct Solution solution) {
    printf("----- reseni ----- \n");
    printf("pocet hran:%d\n", solution.edges);
    printf("graf: \n");
    for (int i = 0; i < solution.n; i++) {
        for (int v = 0; v < solution.n; v++) {
            printf("%d", solution.graph[i][v]);
        }
        printf("\n");
    }
}
