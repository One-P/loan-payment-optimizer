#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "micro-ga.h"
#include "util.h"

// Local functions
static void population_init(micro_ga_t* ga);
static void crossover(	micro_ga_genome_t* mother, micro_ga_genome_t* father,
						micro_ga_genome_t* child, unsigned long int genome_size,
						float crossover_rate);
static void mutate(micro_ga_genome_t* individual, float mutation_rate);
static int genome_compare(const void* genome1, const void *genome2);

int micro_ga_init(micro_ga_t* ga, micro_ga_config_t* config)
{
	unsigned int n;

	// Check required parameters
	if(ga == NULL || config == NULL)
		return -1;
	if(	config->population_size == 0 ||
		config->genome_size == 0     ||
		config->fitness_fn == NULL   ||
		config->mutation_rate < 0    || 
		config->crossover_rate < 0   || 
		config->fitness_thresh < 0 )
	{
		return -2;
	}

	memset(ga, 0, sizeof(micro_ga_t));

	// Copy stuff over from config
	ga->individuals     = NULL;
	ga->genome_size     = config->genome_size;
	ga->population_size = config->population_size;
	ga->mutation_rate   = config->mutation_rate;
	ga->crossover_rate  = config->crossover_rate;
	ga->fitness_thresh  = config->fitness_thresh;
	ga->fitness_fn      = config->fitness_fn;
	ga->acceptance_fn   = config->acceptance_fn;

	// Allocate the population	
	ga->individuals = (micro_ga_genome_t*)calloc(ga->population_size, sizeof(micro_ga_genome_t));
	if(ga->individuals == NULL) {
		return -1;
	}
	for(n = 0; n < ga->population_size; n++) {
		ga->individuals[n].genome_size = ga->genome_size;
		ga->individuals[n].fitness = -1.0;
		ga->individuals[n].genes = (float*)calloc(ga->genome_size, sizeof(float));
	}

	// Initialize population with random genes
	population_init(ga);

	// Everything is ready to use
	ga->ready = 1;

	return 0;
}

int micro_ga_destroy(micro_ga_t* ga)
{
	unsigned int n;

	if(ga == NULL)
		return -1;
	if(ga->ready != 1)
		return -1;

	// Deallocate population
	if(ga->individuals != NULL) {
		for(n = 0; n < ga->population_size; n++) {
			free(ga->individuals[n].genes);
		}
		free(ga->individuals);
	}

	// No longer ready to be run
	ga->ready = 0;

	return 0;
}

void micro_ga_evolve(micro_ga_t* ga)
{
	unsigned int n, x, replace, pcount, nchildren;
	unsigned int* parents;
	double tfitness;
	double* prob;
	double r1, r2, cumulative;
	micro_ga_genome_t *mother, *father, *child;
	micro_ga_genome_t* children;
	
	// Initialized?
	assert(ga->ready == 1);

	// Fitness function valid?
	assert(ga->fitness_fn != NULL);

	// Get population fitness from external function
	for(n = 0; n < ga->population_size; n++) {
		ga->fitness_fn( &(ga->individuals[n]) );
	}

	// Need to allocate the storage for the index of the parent
	// we choose for breeding. Worst case is that all members of the 
	// population are below the replacement threshold, and we must replace
	// all individuals. Thus, size array to be same as total # of individuals
	prob = (double*)calloc(ga->population_size, sizeof(double));
	parents = (unsigned int*)calloc(ga->population_size * 2, sizeof(unsigned int));
	
	// Allocate space for the generated children which replace the unfit
	// individuals. This is necessary so that individuals which will be replaced
	// can still be used for breeding the replacements.
	children = (micro_ga_genome_t*)calloc(ga->population_size, sizeof(micro_ga_genome_t));
	for(n = 0; n < ga->population_size; n++) {
		children[n].genes = (float*)calloc(ga->genome_size, sizeof(float));
	}

	if(prob == NULL || parents == NULL || children == NULL) {
		fprintf(stderr, "Could not allocate memory\n");
		//clean_up();
		//abort();
	}

	// Roulette wheel selection with ellitist reinsertion

	// Sort individuals by fitness
	qsort(ga->individuals, ga->population_size, sizeof(micro_ga_genome_t), &genome_compare);

/*
	for(n = 0; n < ga->population_size; n++) {
		printf("%d\t%f\n", n, ga->individuals[n].fitness);
	}
	puts("\n");
*/

	// Get sum of all individuals' fitnesses
	tfitness = 0;
	for(n = 0; n < ga->population_size; n++)
		tfitness += ga->individuals[n].fitness;

	// Get cumulative probability distribution
	// Remember that the individuals are now in order 
	// from least to greatest fitness
	cumulative = 0;
	for(n = 0; n < ga->population_size; n++) {
		cumulative += ga->individuals[n].fitness / tfitness;
		prob[n] = cumulative;
	}

	replace = ga->population_size - 1;
	if(ga->debug)
		printf("Replace: %d\n", replace);

	// TODO make this more elegant
	// Roulette wheel selection
	pcount = 0;
	for(n = 0; n < replace; )
	{
		// Get mother
		r1 = rand()/((double)RAND_MAX + 1);
		for(x = 0; x < ga->population_size; x++) {
			if(r1 <= prob[x]) {
				parents[pcount] = x;
				break;
			}
		}

		// Get father
		r2 = rand()/((double)RAND_MAX + 1);	
		for(x = 0; x < ga->population_size; x++) {
			if(r2 <= prob[x]) {
				parents[pcount + 1] = x;
				break;
			}
		}

		// Only increment counts if parents are not identical
		if(parents[pcount] != parents[pcount + 1])
		{
			n++;
			pcount += 2;
		}
	}

	if(ga->debug)
	{		
		printf("Parents: ");
		for(n = 0; n < pcount; n++)
			printf("%d ", parents[n]);
		printf("\n");

		printf("Fitness sorted\n");
		for(n = 0; n < ga->population_size; n++)
			printf("%d %f\n", n, ga->individuals[n].fitness);
	}

	// Breed!
	unsigned int m;
	for(n = 0, nchildren = 0; n < replace*2; n+=2, nchildren++)
	{
		mother = &( ga->individuals[ parents[n]   ] );
		father = &( ga->individuals[ parents[n+1] ] );
		child  = &( children[nchildren] );
		crossover(mother, father, child, ga->genome_size, ga->crossover_rate);

		if(ga->debug)
		{
			printf("Mother:\t%d\t", parents[n]);
			for(m = 0; m < ga->genome_size; m++) {
				printf("%.4f ", mother->genes[m]);
			}
			printf("\n");

			printf("Father:\t%d\t", parents[n+1]);
			for(m = 0; m < ga->genome_size; m++) {
				printf("%.4f ", father->genes[m]);
			}
			printf("\n");

			printf("Child:\t\t");
			for(m = 0; m < ga->genome_size; m++) {
				printf("%.4f ", child->genes[m]);
			}
			printf("\n");
		}

	}

	// Mutate!
	for(n = 0; n < replace; n++) {
		mutate( &(children[n]), ga->mutation_rate);
	}


	// Replace the lowest ranking individuals in the original population
	// with the newly created children
	for(n = 0; n < replace; n++)
	{
		// Copy genome size over
		ga->individuals[n].genome_size = children[n].genome_size;

		// Deep-copy over all the genes
		memcpy( ga->individuals[n].genes,
				children[n].genes,
				sizeof(float) * ga->individuals[n].genome_size );

		// Fitness of new individual is unknown!
		ga->individuals[n].fitness = -1.0;
	}

	// Free memory
	for(n = 0; n < ga->population_size; n++) {
		free(children[n].genes);
	}
	free(children);
	free(parents);
	free(prob);
}

void micro_ga_sort(micro_ga_t* ga)
{
	qsort(	ga->individuals, 
			ga->population_size,
			sizeof(micro_ga_genome_t),
			&genome_compare );
}

void micro_ga_print_genome(micro_ga_genome_t* g)
{
	int n;

	if(g == NULL)
		return;

	printf("Genome {\n");
	printf("Fitness:\t%f\n", g->fitness);
	printf("Genome Size:\t%lu\n", g->genome_size);
	printf("Gene values: \n  ");

	for(n = 0; n < g->genome_size; n++) {
		printf("%f\t", g->genes[n]);
		if((n+1) % 3 == 0)
			printf("\n  ");
	}

	printf("\n}\n");
}

static void crossover(	micro_ga_genome_t* mother, micro_ga_genome_t* father,
						micro_ga_genome_t* child, unsigned long int genome_size,
						float crossover_rate)
{
	unsigned int n;
	float c;

	// All family members here?
	assert(mother != NULL && father != NULL && child != NULL);

	assert(genome_size > 0);

	// Birds and the bees...
	for(n = 0; n < genome_size; n++) {
		// Randomly decide if this gene will be crossed over or not
		c = rand()/((float)RAND_MAX + 1);
		if(c > crossover_rate)
		{
			// Generate a new random number which determines how 
			// much the genes will blend
			c = rand()/((float)RAND_MAX + 1);
			child->genes[n] = c*mother->genes[n] + (1.0f-c)*father->genes[n];
		} else {
			c = rand()/((float)RAND_MAX + 1);
			if(c > 0.5)
				child->genes[n] = mother->genes[n];
			else
				child->genes[n] = father->genes[n];
		}
	}

	// Fitness of child is unknown, genome size is the same
	child->fitness = -1.0;
	child->genome_size = genome_size;
}

static void mutate(micro_ga_genome_t* individual, float mutation_rate)
{
	unsigned int mut = 0;
	unsigned int g;
	double r;
	
	for(g = 0; g < individual->genome_size; g++)
	{
		// Mutate?
		r = rand()/((double)RAND_MAX + 1);
		if(r < mutation_rate) {
			// Generate another random number for new gene value
			r = rand()/((double)RAND_MAX + 1);
			individual->genes[g] = r;
		}
	}
}

static void population_init(micro_ga_t* ga)
{
	unsigned int n, g, acceptable;

	// Each individual
	for(n = 0; n < ga->population_size; n++)
	{
		// Each gene
		for(g = 0; g < ga->individuals[n].genome_size; g++)
		{
			// Get a random number between 0 and 1
			ga->individuals[n].genes[g] = rand()/((float)RAND_MAX + 1);
		}
		// Is this solution acceptable to go into the population?
		if(ga->acceptance_fn != NULL) {
			acceptable = ga->acceptance_fn( &(ga->individuals[n]) );
			// If this solution is unacceptable, decrease n by one
			// This effectively generates another solution
			if(acceptable == 0) {
				n--;
			}
		}
	}
}

static int genome_compare(const void* genome1, const void *genome2) 
{
	float fitness1 = ((micro_ga_genome_t*)genome1)->fitness;
	float fitness2 = ((micro_ga_genome_t*)genome2)->fitness;
	
	if(fitness1 < fitness2) {
		return -1;
	} else if(fitness1 > fitness2) {
		return 1;
	} else {
		return 0;
	}
}

