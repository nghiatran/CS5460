
#define MAX_WORK_SIZE 1000
#define MAX_RESULT_SIZE 1000

struct userdef_work_t; /* definition provided by user */
struct userdef_result_t; /* definition provided by user */
typedef struct userdef_work_t mw_work_t;
typedef struct userdef_result_t mw_result_t;

struct mw_api_spec {
   mw_work_t **(*create) (int argc, char **argv);
      /* create work: return a NULL-terminated list of work. Return NULL if it fails. */

   int (*result) (int sz, mw_result_t *res);     
      /* process result. Input is a collection of results, of size sz. Returns 1 on success, 0 on failure. */

   mw_result_t *(*compute) (mw_work_t *work);   
      /* compute, returning NULL if there is no result, non-NULL if there is a result to be returned. */

   int work_sz, res_sz;
      /* size in bytes of the work structure and result structure, needed to send/receive messages */
};

/* run master-worker */
void MW_Run (int argc, char **argv, struct mw_api_spec *f){

  int rank, size;
  MPI_Comm mw_comm;

  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  //MPI_Comm_split( MPI_COMM_WORLD, rank == 0, 0, &mw_comm );
  if (rank == 0) 
    master( MPI_COMM_WORLD, argc, argv, f);
  else
    slave( MPI_COMM_WORLD, f);
}


char * serialize_works(int nWorks, mw_work_t ** works, int work_sz){
  char * b = malloc(work_sz*nWorks);
  for(int i =0;i<nWorks;i++){
    memcpy(b +i*work_sz, *(works+i), work_sz);
  }
  return b;
}

mw_work_t * deserialize_work(char * buff, int work_sz){
  mw_work_t * works = malloc(work_sz);
  memcpy(works, buff, work_sz);
  return works;
}

int master( MPI_Comm global_comm, int argc, char** argv, struct mw_api_spec *f )
{ 
    int size;
    printf("Hello, I am a master\n");

    MPI_Comm_size(global_comm, &size );    
    mw_work_t ** works = f->create(argc, argv);


    int totalWorks=0;
    while(*(works+totalWorks) != NULL)
      totalWorks++;

    // send works to workers
    int a = totalWorks/(size-1);
    for(int worker_rank =1;worker_rank<size;worker_rank++){
      int count = worker_rank == size-1 ? totalWorks - (size-2)*a : a;
      char * b = serialize_works(count, works + (worker_rank-1)*a, f->work_sz);

      MPI_Send(&count,1,MPI_INT,worker_rank,0,global_comm);
      MPI_Send(b, count*f->work_sz, MPI_BYTE,worker_rank,0,global_comm);
      free(b);
    }

    //// collect results
    mw_result_t * mw_results = malloc(f->res_sz*totalWorks);
    char result_buf[f->res_sz];
    MPI_Status status;

    int n=0;
    for(int worker_rank=1;worker_rank<size;worker_rank++){
      int count = worker_rank == size-1 ? totalWorks - (size-2)*a : a;
      for(int i =0;i<count;i++){
        MPI_Recv(result_buf,f->res_sz,MPI_BYTE,worker_rank,0,global_comm,&status);
        memcpy(((char *)mw_results) + n*f->res_sz, result_buf,f->res_sz);
        n++;
      }
    }
    
    f->result(totalWorks,mw_results);

    free(works);
    free(mw_results);
}


int slave(MPI_Comm global_comm, struct mw_api_spec *f )
{
    int  rank;
    MPI_Status status;
    int nWorks;

    MPI_Comm_rank(global_comm, &rank);
    MPI_Recv(&nWorks,1,MPI_INT,0,0,global_comm,&status);
    char buf[nWorks * f->work_sz];
    MPI_Recv(buf,nWorks*(f->work_sz),MPI_BYTE,0,0,global_comm, &status);
    
    //// execute works
    mw_result_t ** results = malloc(sizeof(mw_result_t *) * nWorks);
    for(int i = 0;i<nWorks;i++){
      printf("doing work in process %d:\n",rank);
      mw_work_t * work = deserialize_work(buf+i*(f->work_sz),f->work_sz);
      *(results +i) = f->compute(work);
      free(work);
    }
    
    //// send results to the master
    char result_buf[f->res_sz];    
    for(int i =0;i<nWorks;i++){
      memcpy(result_buf, *(results+i),f->res_sz);
      MPI_Send(result_buf,f->res_sz,MPI_BYTE,0,0,global_comm);
    }
    return 0;
}