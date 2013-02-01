#ifndef MICRO_GA_
#define MICRO_GA_

typedef struct
{
	unsigned long int genome_size;
	float* genes;
	float fitness;
} micro_ga_genome_t;

typedef struct
{
	unsigned int population_size;	/// Total # of individuals in population
	unsigned long int genome_size;	/// Size of all individuals' genome string
	float mutation_rate;			/// Rate of mutation [0:1]
	float crossover_rate;			/// Genetic combination ratio [0:1]
	float fitness_thresh;			/// Individuals replaced if below this [0:1]
	void (*fitness_fn)(micro_ga_genome_t* individual);	
	unsigned int (*acceptance_fn)(micro_ga_genome_t* individual);
	unsigned int debug;
} micro_ga_config_t;

typedef struct
{
	// Population info
	unsigned int generation;		/// Generation number (0, 1, 2...)
	unsigned int population_size;
	float mutation_rate;
	float crossover_rate;
	float fitness_thresh;

	// Individuals
	micro_ga_genome_t* individuals;	/// All individuals in population
	unsigned long int genome_size;	/// Size of all individuals' genome string

	// 
	void (*fitness_fn)(micro_ga_genome_t* individual);	
	unsigned int (*acceptance_fn)(micro_ga_genome_t* individual);

	// Ready flag, everything is properly initialized
	unsigned int ready;

	// Debug print level
	unsigned int debug;

} micro_ga_t;


/** 
 *  
 *  @param ga 
 *  @param config
 *  @return 0 = success, -1 = failure (allocation error or invalid input pointer)
 */
int micro_ga_init(micro_ga_t* ga, micro_ga_config_t* config);

/** 
 *  
 *  @param ga GA to destroy
 *  @return 0 = success, -1 = failure (invalid pointer or input was uninitialized)
 */
int micro_ga_destroy(micro_ga_t* ga);

void micro_ga_evolve(micro_ga_t* ga);

void micro_ga_sort(micro_ga_t* ga);

void micro_ga_print_genome(micro_ga_genome_t* g);

#endif
