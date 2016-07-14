/*
 * Loan payment optimization via genetic algorithms
 * 
 * Assumptions:
 *  - You have > 1 loan to pay off (not useful for one loan)
 *  - You want to optimize the total amount paid over the course of the loan
 *  - Montly payments are a fixed amount
 * 
 * Zachery Shivers <zachery.shivers@gmail.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include "micro-ga.h"


/* Total amount per month you are willing to pay */
#define PAYMENT_NOMINAL			1250.00

/* 
 * How much you're willing to deviate from the nominal montly payment above.
 * If non-zero, the GA will also try to optimize the monthly payment amount.
 * Use zero if you want to only pay exactly PAYMENT_NOMINAL per month.
 */
#define PAYMENT_DEVIATION		0.0


typedef struct {
	float interest_rate;
	float principal;
} loan_t;

/* Number of loans defined in the struct below */
#define NUM_LOANS	3

/* Loan data. Only require the interest rate and the initial principal amount */
loan_t loans[NUM_LOANS] = 
{
	// Loan 1
	{ .interest_rate = 5.00, .principal = 1500.00 },

	// Loan 2
	{ .interest_rate = 3.50, .principal = 10000.00 },

	// Loan 3
	{ .interest_rate = 9.50, .principal = 5000.00 },

};

/* 
 * Maximum number of evolutions the GA will perform. Generally, the
 * higher the number, the better the solution. Too few iterations will lead to 
 * very sub-optimal solutions. Too many iterations will cause longer execution.
 */
#define MAX_ITERATIONS		50

/*
 * Number of individuals in the GA's gene pool. Since this uses a micro GA,
 * the this should be small - somewhere between 5 and 100.
 */
#define POP_SIZE 			15

/* To print out extra debug-level messages, define to non-zero value */
#define VERBOSE				0


/* Locals */
void eval_fitness(micro_ga_genome_t* individual);
float num_payments(loan_t* loan, double monthly_payment);
float total_paid(loan_t* loan, double monthly_payment);
unsigned int eval_acceptance(micro_ga_genome_t* individual);
void genome_to_payments(micro_ga_genome_t* individual, float* payments);
void print_info(micro_ga_t* ga);

int main(int argc, char* argv[])
{

	printf("Loan Payment Optimization\n");
	printf("-------------------------\n");

	srand(time(NULL));

	// Get minimum possible payment
	float minimum_total_payment = 0;
	unsigned int i;
	for(i = 0; i < NUM_LOANS; i++) {
		minimum_total_payment += loans[i].principal;
	}
	printf("Minimum possible total payment: $%.2f\n", minimum_total_payment);
	printf("\n");

	// GA config
	micro_ga_t ga;
	micro_ga_config_t config = 
	{
		.population_size = POP_SIZE,
		.genome_size     = NUM_LOANS,
		.mutation_rate   = 0.1,
		.crossover_rate  = 0.7,
		.fitness_thresh  = 1.0 / (minimum_total_payment * 1.30),
		.fitness_fn      = &eval_fitness,
		.acceptance_fn   = NULL,
		.debug           = (VERBOSE ? 1 : 0)
	};

	// Init the GA
	assert( micro_ga_init(&ga, &config) == 0 );

	unsigned int n = 0, m = 0;
	float best_fitness;
	float fitness;
	unsigned int best_individual;
	do
	{
		micro_ga_evolve(&ga);

		best_fitness = 0.0;
		for(m = 0; m < ga.population_size; m++)
		{
			eval_fitness(&(ga.individuals[m]));
			fitness = ga.individuals[m].fitness;
			if(fitness > best_fitness) {
				best_fitness = fitness;
				best_individual = m;
			}
		}

	} while(++n < MAX_ITERATIONS);

	// Done, print results
	micro_ga_sort(&ga);
	print_info(&ga);

	// Destroy GA
	micro_ga_destroy(&ga);
}

/*
 * Evaluate the fitness of an individual based on total amount paid over the 
 * course of the loan. Since monthly payments are constant, this is proportional
 * to the time taken to pay the loan. 
 */
void eval_fitness(micro_ga_genome_t* individual)
{
	float f = 0.0, p = 0.0, n = 0.0;
	float payments[NUM_LOANS];

	// Convert genome to montly payment amounts
	genome_to_payments(individual, payments);
	
	if(VERBOSE)
		printf("Fitness eval %f\n", f);

	unsigned int i;
	for(i = 0; i < NUM_LOANS; i++)
	{
		p = total_paid( &(loans[i]), payments[i] );
		n = num_payments( &(loans[i]), payments[i] );

		// Fitness is proportional to total amount paid over all loans
		f += p;

		if(VERBOSE)
			printf("\tGene: %.2f\tMonthly payment: %.2f\n", individual->genes[i], payments[i]);

		// Make sure the loan can be paid off at this amount
		// Loans with montly payments too low will take infinite time to pay off
		if(isnan(p) || isnan(n))
		{
			individual->fitness = 1e-10;
			if(VERBOSE)
				printf("\tBad solution!\n");
			return;
		}
	}

	// Optimize inverse because GA wants to achieve f = 1.0
	f = 1.0f / f;
	individual->fitness = f;
}

/* Compute the total number of payments given the loan and a monthly payment */
float num_payments(loan_t* loan, double monthly_payment)
{
	float i = loan->interest_rate / 12.0 / 100.0;
	float n  = -1 * log10(1 - i * loan->principal / monthly_payment );
	return n /= log10(1 + i);
}

/* Compute the total paid given the loan and a monthly payment */
float total_paid(loan_t* loan, double monthly_payment)
{
	float i = loan->interest_rate / 12.0 / 100.0;
	float n = num_payments(loan, monthly_payment);
	return n * monthly_payment;
}

/* 
 * Compute the total amount to be paid monthly. This amount will be split
 * between all the loans. If PAYMENT_DEVIATION != 0, the loan amount will vary
 * with the last genome in the individual's DNA.
 */
float monthly_nominal(micro_ga_genome_t* individual)
{
	return (float)PAYMENT_NOMINAL + PAYMENT_DEVIATION * (individual->genes[NUM_LOANS - 1]);
}

/*
 * Convert a genome sequence into the monthly payment amount.
 * Since we need to split the total montly payment into pieces that must
 * add up to the total payment, we use the numeric value of the genes as 
 * the amount of the remaining monthly payment to take.
 * 
 * For example, if you have 3 loans, the monthly payment will be divided 2 
 * times in the following way:
 * genes[0] = 0.75, genes[1] = 0.25, payment = $1000
 * 
 *   <------------------------- $1000 ------------------------------>

 *   ===============================================================
 *   |             loan0               |   loan1  |      loan2      |
 *   ===============================================================
 * 
 *   <-------------$750---------------> <-$62.50-> <---$187.50*---->
 * 
 */
void genome_to_payments(micro_ga_genome_t* individual, float* payments)
{
	float remaining = monthly_nominal(individual);
	float p = 0.0;
	unsigned int i;
	for(i = 0; i < NUM_LOANS - 1; i++)
	{
		payments[i] = remaining * individual->genes[i];
		remaining -= payments[i];
	}
	// Last payment is the leftover amount
	payments[NUM_LOANS - 1] = remaining;
}

/* Print information about an individual solution */
void print_info(micro_ga_t* ga)
{
	float t, y;
	float payments[NUM_LOANS];
	unsigned int i, j;

	printf("Summary\n");
	printf("-------\n");

	for(i = 0; i < POP_SIZE; i++)
	{
		t = 0.0;

		// Convert genome to montly payment amounts
		genome_to_payments( &(ga->individuals[i]), payments);

		printf("Individual %u\n", i);
		printf("--------------\n");

		for(j = 0; j< NUM_LOANS; j++) {
			t += total_paid( &(loans[j]), payments[j] );
			y = num_payments( &(loans[j]), payments[j] ) / 12.0;
			printf(" Loan %u:\tPayment: $%.2f\tYears: %.2f\n", j, payments[j], y);
		}

		printf("Monthly Payment: $%.2f\n", monthly_nominal( &(ga->individuals[i]) ));
		printf("Total Paid:      $%.2f\n", t);
		printf("\n");
	}
}

