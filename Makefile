FLAGS = -g -Wall -Wvla -fsanitize=address

%: %.c; gcc $(FLAGS) -o $@ $^;

all: memperf;

clean: ; rm memperf;
