#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

pthread_barrier_t bar;

typedef struct BoardTag {
   int row;     // number of rows
   int col;     // number of columns
   char** src;  // 2D matrix holding the booleans (0 = DEAD , 1 = ALIVE). 
} Board;

typedef struct targs {
    int si;
    int ei;
    int nbi;
    Board* l1;
    Board* l2;
    Board* src;
    Board* out;
} targs;

/**
 * Allocates memory to hold a board of rxc cells. 
 * The board is made thoroidal (donut shaped) by simply wrapping around
 * with modulo arithmetic when going beyond the end of the board.
 */
Board* makeBoard(int r,int c)
{
   Board* p = (Board*)malloc(sizeof(Board));
   p->row = r;
   p->col = c;
   p->src = (char**)malloc(sizeof(char*)*r);
   int i;
   for(i=0;i<r;i++)
      p->src[i] = (char*)malloc(sizeof(char)*c);
   return p;
}
/**
 * Deallocate the memory used to represent a board
 */
void freeBoard(Board* b)
{
   int i;
   for(i=0;i<b->row;i++)
      free(b->src[i]);
   free(b->src);
   free(b);
}

/**
 * Reads a board from a file named `fname`. The routine allocates
 * a board and fills it with the data from the file. 
 */
Board* readBoard(char* fName)
{
   int row,col,i,j;
   FILE* src = fopen(fName,"r");
   fscanf(src,"%d %d\n",&row,&col);
   Board* rv = makeBoard(row,col);
   for(i=0;i<row;i++) {
      for(j=0;j<col;j++) {
         char ch = fgetc(src);
         rv->src[i][j] = ch == '*';
      }
      char skip = fgetc(src);
      while (skip != '\n') skip = fgetc(src);
   }
   fclose(src);   
   return rv;
}

/**
 * Save a board `b` into a FILE pointed to by `fd`
 */
void saveBoard(Board* b,FILE* fd)
{
   int i,j;
   for(i=0;i<b->row;i++) {
      fprintf(fd,"|");
      for(j=0;j < b->col;j++) 
         fprintf(fd,"%c",b->src[i][j] ? '*' : ' ');
      fprintf(fd,"|\n");
   }
}
/**
 * Low-level convenience API to clear a terminal screen
 */
void clearScreen()
{
   static const char *CLEAR_SCREEN_ANSI = "\e[1;1H\e[2J";
   static int l = 10;
   write(STDOUT_FILENO, CLEAR_SCREEN_ANSI, l);
}


void printBoard(Board* b)
{
   clearScreen();	
   saveBoard(b,stdout);
}

/*
 * Simple routine that counts the number of neighbors that
 * are alive around cell (i,j) on board `b`.
 */
int liveNeighbors(int i,int j,Board* b)
{
   const int pc = (j-1) < 0 ? b->col-1 : j - 1;
   const int nc = (j + 1) % b->col;
   const int pr = (i-1) < 0 ? b->row-1 : i - 1;
   const int nr = (i + 1) % b->row;
   int xd[8] = {pc , j , nc,pc, nc, pc , j , nc };
   int yd[8] = {pr , pr, pr,i , i , nr , nr ,nr };
   int ttl = 0;
   int k;
   for(k=0;k < 8;k++)
      ttl += b->src[yd[k]][xd[k]];
   return ttl;
}

/*
 * Sequential routine that writes into the `out` board the 
 * result of evolving the `src` board for one generation.
 */
void evolveBoard(Board* src,Board* out,int rsi,int rei)
{
   static int rule[2][9] = {
      {0,0,0,1,0,0,0,0,0},
      {0,0,1,1,0,0,0,0,0}
   };
   int i,j;
   for(i=rsi;i<rei;i++) {
      for(j=0;j<src->col;j++) {
         int ln = liveNeighbors(i,j,src);
         int c  = src->src[i][j];
         out->src[i][j] = rule[c][ln];
      }
   }
}



pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

targs* divideWork(int nbw,int rows,int nbi,Board* l1,Board* l2){
    targs* args = malloc(sizeof(targs)*nbw);
    int r = rows % nbw;
    int index = 0;
    for (int i = 0;i<nbw;i++){
        args[i].si = index;
        index += rows/nbw;
        args[i].ei = index;
        args[i].nbi = nbi;
        args[i].l1 = l1;
        args[i].l2 = l2;
        //b0 = args[i].src;
        //b1 = args[i].out;
    }
    args[nbw-1].ei += r;
    return args;
}


void* startThread(void* args){
    targs* arg = (targs*) args;
    int g;
   for(g=0; g<arg->nbi; g++){
       pthread_barrier_wait(&bar);
       arg->src = g & 0x1 ? arg->l2 : arg->l1;
       arg->out = g & 0x1 ? arg->l1 : arg->l2;
       evolveBoard(arg->src,arg->out,arg->si,arg->ei);
   }
}

int main(int argc,char* argv[])
{
   if (argc < 3) {
      printf("Usage: lifeMT <dataFile> #iter #workers\n");
      exit(1);
   }
   int nbi = atoi(argv[2]);
   int nbw = atoi(argv[3]);
   Board* life1 = readBoard(argv[1]);
   Board* life2 = makeBoard(life1->row,life1->col);
   Board *b0, *b1;
   targs* args = divideWork(nbw,life1->row,nbi,life1,life2);
   //for (int i = 0;i<nbw;i++){
     //  b0 -> args[i].src;
       //b1 -> args[i].out;
   //}
   pthread_t threads[nbw];
   
   pthread_barrier_init(&bar,NULL,nbw);
   for (int i = 0; i< nbw;i++){
       
       assert(pthread_create(&threads[i], NULL, startThread, &args[i]) == 0);
        }
       for(int i = 0; i < nbw; i++){
       assert(pthread_join(threads[i], NULL) == 0);
        }
        pthread_barrier_destroy(&bar);
   
    //printBoard(b1);
   FILE* final = fopen("final.txt","w");
   //printf("%d b1r\n", b1->col);
   saveBoard(args[0].out,final);
   fclose(final);
   freeBoard(life1);
   freeBoard(life2);
    
   return 0;
} 










