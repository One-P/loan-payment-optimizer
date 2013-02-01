PROGRAM = loan-optimize
PROGRAM_FILES = loan-optimize.c micro-ga.c

CC 	=  gcc
CFLAGS	+= -g
LDFLAGS	+= 
LIBS 	+= -lm

all: $(PROGRAM)

%: %.c 
	$(CC) $(PROGRAM_FILES) $(CFLAGS) $(LDFLAGS) -o $(PROGRAM) $(LIBS)

clean:
	@rm -rf $(PROGRAM)
