.PHONY: all clean

CC= g++
CFLAGS= -Wall -Wextra -Werror -O3 -Wpedantic
OUTPUT_OPTION= -MMD -MP -o $@

SOURCE= sched_demo_313553052.cpp
TARGET= sched_demo_313553052

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	@rm -f $(TARGET)

