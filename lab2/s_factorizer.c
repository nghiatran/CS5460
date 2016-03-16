#include <stdio.h>
#include "mw_api.h"
#include "math.h"
#include <gmp.h>
#include <string.h>
#include "factorizer.h"

int main(int argc, char **argv)
{
  mpz_t val;
  mpz_init(val);
  mpz_set_str(val,argv[1],10);

  mpz_t mp_sqrt;
  mpz_init(mp_sqrt);  
  mpz_sqrt (mp_sqrt, val);


  mpz_t i;
  mpz_init(i);
  mpz_set_str(i,"2",10);

  struct factor_node * factors = find_factors(i,mp_sqrt,val);

  mpz_t count;
  mpz_init(count);
  mpz_set_str(count,"0",10);
  struct factor_node * iter = factors;
  while(iter != NULL){
    iter = iter->next;
    mpz_add_ui(count,count,1);
  }
  printf("The number has %s factors\n",mpz_get_str(NULL,10,count));

  return 0;
}


