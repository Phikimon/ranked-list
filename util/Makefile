CFLAGS=-std=c99 -c -g

all: input_generator output_generator check_output bin_to_text

input_generator: input_generator.o
	$(CC) $(LFLAGS) $< -o $@

output_generator: output_generator.o
	$(CC) $(LFLAGS) $< -o $@

check_output: check_output.o
	$(CC) $(LFLAGS) $< -o $@

bin_to_text: bin_to_text.o
	$(CC) $(LFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $<

clean:
	rm input_generator output_generator check_output bin_to_text *.o text bin solution_bin 2> /dev/null
