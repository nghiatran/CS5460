#include <stdio.h>
#include "mw_api.h"
#include "math.h"
#include <gmp.h>

struct userdef_work_t{
	int val;
	int *divisors;
	int divisorCount;
};


struct userdef_result_t{
	int *divisors;
	int count;
};

mw_result_t *do_work(mw_work_t *work){
	mw_result_t *result = malloc(sizeof(mw_result_t));
	int len = work->divisorCount;
	int cnt = 0;
  result->divisors = malloc(len*sizeof(int));
  printf("len is %d\n",len);
	for(int i=0;i<len;i++){
    printf("potential factor: %d\n",*(work->divisors));    
		if(work->val % (*(work->divisors+i))== 0){
			*(result->divisors + cnt) = *(work->divisors+i);
			cnt += 1;			
		}
	}
	result->count = cnt;
	return result;
}


mw_work_t **create_work(int argc,char **argv){
 	double num = 150; //random. Need to use big numbers
 	
  int sqrtn = (int)sqrt(num);
 	int divisions_per_work = 4;//random

 	mw_work_t ** works = malloc(sizeof(mw_work_t*) * sqrtn);
 	int i=2;

 	for(i=2;i<=sqrtn;i+=divisions_per_work){
 		mw_work_t *wrk = malloc(sizeof(mw_work_t));
 		wrk->val = num;
 		int *divisors = malloc(sizeof(int)*divisions_per_work);
 		for(int j=0;j<divisions_per_work;j++){
 			*(divisors+j) = i+j; 
 		}

 		wrk->divisors = divisors;
 		wrk->divisorCount = divisions_per_work;
 		*(works + ((i-2)/divisions_per_work)) = wrk;
 	}
 	*(works+ ((i-2)/divisions_per_work)) = NULL;

  for(int j=0; *(works+j) != NULL;j++){
    printf("divisor: %d\n",*(*(works+j))->divisors);
  }
 	return works;

}


int * process_results(int sz, mw_result_t *res){
  int factors_count=0;
  for(int i=0;i<sz;i++){
    factors_count += res->count;
    res++;
  }
  mw_result_t * final_result = malloc(sizeof(mw_result_t));
  final_result->divisors = malloc(sizeof(int)*factors_count);
  for(int i=0;i<sz;i++){
    memcpy(final_result + res->count*sizeof(int),res->divisors,sizeof(int));
    res++;
  }

  for(int i=0;i<factors_count;i++)
    printf("%d, ",*(final_result->divisors+i));

  return 1;
}



int main(int argc, char **argv)
{
  struct mw_api_spec f;

  MPI_Init (&argc, &argv);

  f.create = create_work;
  f.result = process_results;
  f.compute = do_work;
  f.work_sz = sizeof (struct userdef_work_t);
  f.res_sz = sizeof (struct userdef_result_t);
  MW_Run (argc, argv, &f);
  MPI_Finalize ();

  return 0;

}