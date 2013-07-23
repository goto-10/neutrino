BIN=bin
OUT=$(BIN)/out
VALGRIND=valgrind -q --leak-check=full


# Default target.
main:	all-c


# Convenience alias for building everything.
all:	main


# Run all tests.
test:	test-python test-c


# Dependencies for all targets.
GLOBAL_DEPS=Makefile


PYTHON_TEST_FILES=$(shell find tests/python -name "[^_]*.py")
PYTHON_TEST_RUNS=$(patsubst tests/python/%, test-python-%, $(PYTHON_TEST_FILES))


# Runs the python tests.
test-python:	$(PYTHON_TEST_RUNS)


# Individual python tests.
$(PYTHON_TEST_RUNS): test-python-%: tests/python/% $(GLOBAL_DEPS)
	@echo Running $<
	@PYTHONPATH=$(PYTHONPATH):src/python \
		python $<


# Configuration of the C language dialect to use.
C_DIALECT_FLAGS=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-function -std=c99 -pedantic
C_ENV_FLAGS=-Isrc/c -I$(BIN)/tests/c
CFLAGS=$(C_DIALECT_FLAGS) $(C_ENV_FLAGS) -O0 -g -m32
LINKFLAGS=


# The library part of ctrino, that is, everything but main.
C_MAIN_NAME=main.c
C_LIB_SRCS=$(shell find src/c -name "*.c" -and -not -name $(C_MAIN_NAME) | sort)
C_LIB_HDRS=$(shell find src/c -name "*.h" | sort)
C_LIB_OBJS=$(patsubst %.c, bin/%.o, $(C_LIB_SRCS))
C_LIB_DEPS=$(C_LIB_HDRS) $(GLOBAL_DEPS)


# The C library object files.
$(C_LIB_OBJS): $(BIN)/%.o: %.c $(C_LIB_DEPS)
	@mkdir -p $(shell dirname $@)
	@echo Compiling $<
	@gcc $(CFLAGS) -c $< -o $@


# The main part of ctrino.
C_MAIN_SRCS=$(shell find src/c -name "*.c" -and -name $(C_MAIN_NAME) | sort)
C_MAIN_OBJS=$(patsubst %.c, bin/%.o, $(C_MAIN_SRCS))
C_MAIN_DEPS=$(C_LIB_DEPS)
C_MAIN_EXE=$(BIN)/ctrino


# The main object files.
$(C_MAIN_OBJS): $(BIN)/%.o: %.c $(C_MAIN_DEPS)
	@mkdir -p $(shell dirname $@)
	@echo Compiling $<
	@gcc $(CFLAGS) -c $< -o $@


# Build the ctrino executable.
$(C_MAIN_EXE): $(C_MAIN_OBJS) $(C_LIB_OBJS)
	@mkdir -p $(shell dirname $@)
	@echo Building $@
	@gcc $(LINKFLAGS) $^ -o $@


# The library parts of the tests, that it, everything but the test main.
C_TEST_MAIN_NAME=test.c
C_TEST_LIB_SRCS=$(shell find tests/c -name "*.c" -and -not -name $(C_TEST_MAIN_NAME) | sort)
C_TEST_LIB_HDRS=$(shell find tests/c -name "*.h" | sort)
C_TEST_LIB_OBJS=$(patsubst %.c, $(BIN)/%.o, $(C_TEST_LIB_SRCS))
C_TEST_LIB_RUNS=$(patsubst tests/c/test_%.c, test-c-%, $(C_TEST_LIB_SRCS))
C_TEST_LIB_OUTS=$(patsubst tests/c/test_%.c, $(OUT)/tests/c/test_%.out, $(C_TEST_LIB_SRCS))
C_TEST_LIB_DEPS=$(C_LIB_DEPS) $(C_TEST_LIB_HDRS)


# The test object files.
$(C_TEST_LIB_OBJS): $(BIN)/%.o: %.c $(C_TEST_LIB_DEPS)
	@mkdir -p $(shell dirname $@)
	@echo Compiling $<
	@gcc $(CFLAGS) -c $< -o $@


# The generated table of contents
C_TEST_TOC_SRCS=$(BIN)/tests/c/toc.c


# Build the table of contents. Yuck.
$(C_TEST_TOC_SRCS):	$(C_TEST_LIB_SRCS) $(GLOBAL_DEPS)
	@mkdir -p $(shell dirname $@)
	@echo Generating test table of contents
	@cat $(C_TEST_LIB_SRCS) \
	  | grep "TEST\(.*\)" \
	  | sed "s/\(TEST(.*)\).*/DECLARE_\1;/g" \
	  > $@
	@echo "ENUMERATE_TESTS_HEADER {" \
	  >> $@
	@cat $(C_TEST_LIB_SRCS) \
	  | grep "TEST\(.*\)" \
	  | sed "s/\(TEST(.*)\).*/  ENUMERATE_\1;/g" \
	  >> $@
	@echo "}" \
	  >> $@


# The main test runner.
C_TEST_MAIN_SRCS=$(shell find tests/c -name "*.c" -and -name $(C_TEST_MAIN_NAME) | sort)
C_TEST_MAIN_OBJS=$(patsubst %.c, $(BIN)/%.o, $(C_TEST_MAIN_SRCS))
C_TEST_MAIN_DEPS=$(C_TEST_LIB_DEPS) $(C_TEST_TOC_SRCS)
C_TEST_MAIN_EXE=$(BIN)/tests/c/main


# The main object files.
$(C_TEST_MAIN_OBJS): $(BIN)/%.o: %.c $(C_TEST_MAIN_DEPS)
	@mkdir -p $(shell dirname $@)
	@echo Compiling $<
	@gcc $(CFLAGS) -c $< -o $@


# Build all the tests into one executable.
$(C_TEST_MAIN_EXE): $(C_TEST_MAIN_OBJS) $(C_TEST_LIB_OBJS) $(C_LIB_OBJS)
	@mkdir -p $(shell dirname $@)
	@echo Building $@
	@gcc $(LINKFLAGS) $^ -o $@


# Run a C test and store the result in a file. This is kind of tricky because
# we want to both store the output and signal an error 
$(C_TEST_LIB_OUTS):$(OUT)/tests/c/test_%.out:$(C_TEST_MAIN_EXE)
	@echo Running test_$*
	@mkdir -p $(shell dirname $@)
	@$(VALGRIND) ./$(C_TEST_MAIN_EXE) $* > $@ || touch $@.fail
	@cat $@
	@if [ -f $@.fail ]; then rm $@ $@.fail; false; else true; fi


# Shorthand for running a C test.
$(C_TEST_LIB_RUNS):test-c-%:$(OUT)/tests/c/test_%.out


# Shorthand for running all the C tests.
test-c:$(C_TEST_LIB_RUNS)


# Build everything.
all-c:	$(C_MAIN_EXE) $(C_TEST_MAIN_EXE)


clean:
	@echo Cleaning $(BIN)
	@rm -rf $(BIN)


.PHONY:	clean tests-c tests-python $(C_TEST_LIB_RUNS) $(PYTHON_TEST_RUNS)
