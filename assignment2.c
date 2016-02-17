/***************************************************************************/
/* Template for Asssignment 1 **********************************************/
/* Your Name Here             **********************************************/
/***************************************************************************/

/***************************************************************************/
/* Includes ****************************************************************/
/***************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<math.h>
// New Includes for Assignment 2
#include"clcg4.h"
#include<mpi.h>


/***************************************************************************/
/* Defines *****************************************************************/
/***************************************************************************/

#define ALIVE 1
#define DEAD  0

/***************************************************************************/
/* Global Vars *************************************************************/
/***************************************************************************/

// Bring over from your code
#define NROWS 1024 

unsigned int **g_GOL_CELL=NULL;
unsigned int **g_GOL_CELL_TMP=NULL;
unsigned int g_x_cell_size=0;
unsigned int g_y_cell_size=0;
unsigned int g_num_ticks=100;
double THRESHOLD = 0.25;

int mpi_myrank;
int mpi_commsize;


/***************************************************************************/
/* Function Decs ***********************************************************/
/***************************************************************************/

// Bring over from your code

void allocate_and_init_cells();
void compute_one_tick();
void output_final_cell_state(int t);


/***************************************************************************/
/* Function: Main **********************************************************/
/***************************************************************************/

int main(int argc, char *argv[])
{
    double starttime = 0, endtime = 0;
    int top, bottom;
    unsigned int rbuffer_top[NROWS], rbuffer_bottom[NROWS];
    unsigned int sbuffer_top[NROWS], sbuffer_bottom[NROWS];
    MPI_Request srequest_top, srequest_bottom, rrequest_top, rrequest_bottom;
    MPI_Status status;

    int i = 0;
    int j = 0;
// Example MPI startup and using CLCG4 RNG
    MPI_Init( &argc, &argv);
    MPI_Comm_size( MPI_COMM_WORLD, &mpi_commsize);
    MPI_Comm_rank( MPI_COMM_WORLD, &mpi_myrank);

    if (mpi_myrank == 0)  starttime = MPI_Wtime();
    
// Init 16,384 RNG streams - each rank has an independent stream
    InitDefault();
    
// Note, use the mpi_myrank to select which RNG stream to use. 
    printf("Rank %d of %d has been started and a first Random Value of %lf\n", 
	   mpi_myrank, mpi_commsize, GenVal(mpi_myrank));
    
    MPI_Barrier( MPI_COMM_WORLD );
// Bring over rest from your code
//Initialization
    bottom = (mpi_myrank + 1) % mpi_commsize;
    top = ( mpi_myrank + mpi_commsize - 1 ) % mpi_commsize;

    g_y_cell_size = NROWS / mpi_commsize + 2; // 2 extra ghost rows  
    g_x_cell_size = NROWS; 

    allocate_and_init_cells();

    for ( i = 0;i < g_num_ticks; ++i)
    {
//Communication

      //memcpy(sbuffer_top,  (char *)g_GOL_CELL + 1 * g_x_cell_size * sizeof(unsigned int), g_x_cell_size * sizeof(unsigned int)); 
      //memcpy(sbuffer_bottom,  (char *)g_GOL_CELL + (g_y_cell_size - 2) * g_x_cell_size * sizeof(unsigned int), g_x_cell_size * sizeof(unsigned int)); 

      for (j = 0;j < g_x_cell_size; ++j)
      {
        sbuffer_top[j] = g_GOL_CELL[j][1];
        sbuffer_bottom[j] = g_GOL_CELL[j][g_y_cell_size - 2];
      }

      MPI_Irecv(rbuffer_top, NROWS , MPI_UNSIGNED, top, 123, MPI_COMM_WORLD, &rrequest_top);
      MPI_Irecv(rbuffer_bottom, NROWS , MPI_UNSIGNED, bottom, 123, MPI_COMM_WORLD, &rrequest_bottom);
      MPI_Isend(sbuffer_top, NROWS, MPI_UNSIGNED, top, 123, MPI_COMM_WORLD, &srequest_top);
      MPI_Isend(sbuffer_bottom, NROWS, MPI_UNSIGNED, bottom, 123, MPI_COMM_WORLD, &srequest_bottom);
      MPI_Wait(&rrequest_top, &status);
      MPI_Wait(&rrequest_bottom, &status);
      MPI_Wait(&srequest_top, &status);
      MPI_Wait(&srequest_bottom, &status);
      //printf("Rank %d of %d received top%d,bottom%d in tick %d\n", 
      //         mpi_myrank, mpi_commsize, rbuffer_top[0], rbuffer_bottom[0], i);

      for (j = 0;j < g_x_cell_size; ++j)
      {
        g_GOL_CELL[j][0] = rbuffer_top[j];
        g_GOL_CELL[j][g_y_cell_size - 1] = rbuffer_bottom[j];
      }

      //memcpy(g_GOL_CELL, rbuffer_top, g_x_cell_size * sizeof(unsigned int)); 
      //memcpy((char *)g_GOL_CELL + (g_y_cell_size - 1) * g_x_cell_size * sizeof(unsigned int), (char *)rbuffer_bottom, g_x_cell_size * sizeof(unsigned int)); 

//Game of Life Simulation(serial)
      compute_one_tick();
    }

    //output_final_cell_state(i);

// END -Perform a barrier and then leave MPI
    MPI_Barrier( MPI_COMM_WORLD );
    MPI_Finalize();

    if (mpi_myrank == 0)
    {
      endtime = MPI_Wtime();
      printf("That took %f seconds\n",endtime-starttime);
    }
    return 0;
}

/***************************************************************************/
/* Other Functions - Bring over from your Assignment 1 *********************/
/***************************************************************************/

/***************************************************************************/
/* Function: allocate_and_init_cells ***************************************/
/***************************************************************************/

void allocate_and_init_cells()
{
  // use "calloc" to allocate space for your cell matrix
  int i = 0, j= 0;
  g_GOL_CELL = calloc(g_x_cell_size , 1 + sizeof(unsigned int*)); // alloc one extra ptr
  g_GOL_CELL_TMP = calloc(g_x_cell_size , 1 + sizeof(unsigned int*)); // alloc one extra ptr
  for (i = 0;i < g_x_cell_size;i++) 
  {
    g_GOL_CELL[i] = calloc(g_y_cell_size, sizeof(unsigned int));
    g_GOL_CELL_TMP[i] = calloc(g_y_cell_size, sizeof(unsigned int));
    for (j = 0;j < g_y_cell_size;++j)
    {
      g_GOL_CELL[i][j] = GenVal(mpi_myrank) < 0.500 ? ALIVE : DEAD; 
    }
  }
  g_GOL_CELL[g_x_cell_size] = NULL; // set the extra ptr to NULL
  g_GOL_CELL_TMP[g_x_cell_size] = NULL; // set the extra ptr to NULL
}

/***************************************************************************/
/* Function: compute_one_tick **********************************************/
/***************************************************************************/

void compute_one_tick()
{
  // iterate over X (outside loop) and Y (inside loop) dimensions of the g_GOL_CELL
  int i = 0, j=0;
  int ii = 0, jj = 0, alives =0, mortals = 0;
  for (i = 0;i < g_x_cell_size;++i)
  {
    for ( j = 0;j < g_y_cell_size;++j)
    {
      if ( GenVal(mpi_myrank) > THRESHOLD )
      {
        alives = 0;
        mortals = 0;
        for (ii = i-1;ii <= i+1; ii++)
        {
          for (jj = j-1;jj <= j+1; jj++)
          {
            if ( !(ii == i && jj == j) && ii >= 0 && ii < g_x_cell_size && jj >= 0 && jj < g_y_cell_size)
            {
              alives += (g_GOL_CELL[ii][jj] == ALIVE) ? 1 : 0;
              mortals += (g_GOL_CELL[ii][jj] == DEAD)? 1 : 0;
            }
          }
        }
        if (g_GOL_CELL[i][j] == ALIVE && alives < 2)
          g_GOL_CELL_TMP[i][j] = DEAD;
        else if(g_GOL_CELL[i][j] == ALIVE && (alives == 2 || alives == 3))
          g_GOL_CELL_TMP[i][j] = ALIVE;
        else if(g_GOL_CELL[i][j] == ALIVE && alives > 3)
          g_GOL_CELL_TMP[i][j] = DEAD;
        else if(g_GOL_CELL[i][j] == DEAD && alives == 3)
          g_GOL_CELL_TMP[i][j] = ALIVE;
      }
      else
      {
        if( GenVal(mpi_myrank) > 0.5000 )
          g_GOL_CELL[i][j] = ALIVE;
        else
          g_GOL_CELL[i][j] = DEAD;
      }
    }
  }
  for (i = 0;i < g_x_cell_size;++i)
  {
    for ( j = 0;j < g_y_cell_size;++j)
    {
          g_GOL_CELL[i][j] = g_GOL_CELL_TMP[i][j]; 

    }
  }
}


/***************************************************************************/
/* Function: output_final_cell_state ***************************************/
/***************************************************************************/

void output_final_cell_state(int t)
{
  char filename[256];
  sprintf(filename, "%0.2fthersdhold_rank%dof%d_size%d*%d.txt", 
                          THRESHOLD, mpi_myrank, mpi_commsize, g_x_cell_size, NROWS);
  FILE * fp = fopen(filename, "w+");
  if (fp == NULL) perror("FILE OPEN FAILED");
  
  // print out in grid form 16 value per row of the g_GOL_CELL
  // This data will be used to create your graphs
  int i = 0, j=0;
  //fprintf(fp, "[");
  for (i = 1; i < g_y_cell_size -1 ;++i)
  {
    for ( j = 0;j < g_x_cell_size - 1;++j)
    {
      fprintf(fp, "%u ", g_GOL_CELL[j][i]);
    }
    fprintf(fp, "%u;\\\n ", g_GOL_CELL[j][g_x_cell_size - 1]);
  }
  //fprintf(fp, "]%d*%d\n", g_x_cell_size, g_y_cell_size);
  fclose(fp);
}
