Loan Payment Optimizer
===================

Use a genetic algorithm to optimize loan payments across several loans. Useful
for people with multiple loans with different interest rates and principals.
Especially useful for people with large loans from a large number of creditors.

This program will try to produce loan payment amounts which minimize the overall
cost of the loan over its lifetime.

Assumptions
----------
- You have > 1 loan to pay off (not useful for one loan)
- You want to optimize the total amount paid over the course of the loan
- Monthly payments are a fixed amount over the loan lifetime (typical)
- Once a loan is paid off, your total monthly payment for all the loans is
  decreased. For example, if you pay $100 / month on all loans, and you pay
  off one of the loans that was $30 / month, your monthly payment now is just
  for the remaining loan ($70 / month).
  More realistically, you may want to divert the extra $30 / month to the
  remaining loan. This would reduce the total loan repayment time and cost.
  Future versions will account for this.

Usage
----
You need to input your loans' parameters into the program by manually editing
loan_optimize.c

Example:
You have the following 3 loans:
 1. Principal of  1,500 @ 5.0% interest
 2. Principal of 10,000 @ 3.5% interest
 3. Principal of  5,000 @ 9.5% interest
You're willing to pay up to 1,250 per month for all loans.

Open up loan_optimize.c and take a quick look around. You'll be editing the 
following values.

Put in the amount you're willing to pay into PAYMENT_NOMINAL, like so:
#define PAYMENT_NOMINAL			1250.00

Put in the total number of loans you're trying to optimize into NUM_LOANS:
#define NUM_LOANS	  3

For each loan, edit the struct initializer:
loan_t loans[NUM_LOANS] = 
{
	{ .interest_rate = 5.00, .principal = 1500.00 },
	{ .interest_rate = 3.50, .principal = 10000.00 },
	{ .interest_rate = 9.50, .principal = 5000.00 },
};

Finally, open up a terminal and compile it using Make:
> make

Now run that baby!
> ./loan_optimize

You should now see a long summary of each of the individual's performance
at optimizing you loan payments.

Individuals represent a set of loan payment amounts. The summary of the 
individuals sorts worst to best, so:
  The *best* performing individual appears at the end of the list.

Enjoy your computer-optimized financial future! :D

Genetic Algos
-----------
Genetic algorithms are really freakin' cool and can be used on a wide array of
optimization problems. I've implemented a so-called "micro-GA" which means that
the number of individuals simultaneously evolving is held small.

The fitness funtion used to judge the quality of an individual's solution is
the total amount paid on all loans (we'd like to pay less money, right?).

When solutions are bred (that is, two solutions have sexy time), I decided
to use the common roulette method for combining their genetic material.

You can use micro-ga.c/h in your own optimization program, too! It's not 
well-documented here, but I plan on releasing a better version on GitHub soon.


Disclaimer
---------
Genetic algorithms are tricky things - they don't guarantee optimum solutions.
The point of this program is to get you a good approximation to the optimum
solution.

!! This code is licensed as BSD; use at your own risk!
!! Please don't blame me if you something bad happens when you blindly use 
!! these values as your loan payments. Always double check the computations
!! before making large financial decisions!

TODO
---
- Use files as input instead of having to recompile when loan parameters change
- Account for diverting monthly payments from paid off loans to remaining loans
- Clean up micro-ga library
- Add minimum payment amounts for each loan

