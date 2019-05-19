#------------------------------------------------------------------------------

UNIPROC_OBJ=uniprocessor_exp.o task.o taskset.o taskset-gen.o request-driven-test.o job-driven-test.o hybrid-test.o indirect-cis.o config.hpp
#MCPROC_OBJ=mcprocessor_exp.o task_partitioning.o cycle-tandem.o cycle-solo-cpu.o cycle-solo-accel.o cycle-solo-cpu-mc.o cycle-solo-accel-mc.o cycle-solo-id-mc.o task.o taskset.o taskset-gen.o request-driven-test.o binary-search.o energy.o config.hpp
#MCPROC_SCHED_OBJ=mcprocessor_sched_exp.o task_partitioning.o cycle-tandem.o cycle-solo-cpu.o cycle-solo-accel.o cycle-solo-cpu-mc.o cycle-solo-accel-mc.o cycle-solo-id-mc.o task.o taskset.o taskset-gen.o request-driven-test.o binary-search.o energy.o config.hpp
#PAPEREX_OBJ=paper_examples.o cycle-tandem.o cycle-solo-cpu.o cycle-solo-accel.o cycle-solo-id.o task.o taskset.o taskset-gen.o request-driven-test.o binary-search.o energy.o config.hpp
CFLAGS_NEW=-std=c++0x
MYLIBRARIES=m
CC=g++

#------------------------------------------------------------------------------

all: uniprocessor_exp #mcprocessor_exp mcprocessor_sched_exp paper_examples

task.o: task.cpp task.hpp config.hpp
	$(CC) -c task.cpp -o task.o -l$(MYLIBRARIES) $(CFLAGS_NEW)

taskset.o: taskset.cpp taskset.hpp config.hpp
	$(CC) -c taskset.cpp -o taskset.o -l$(MYLIBRARIES) $(CFLAGS_NEW)

taskset-gen.o: taskset-gen.cpp taskset-gen.hpp config.hpp
	$(CC) -c taskset-gen.cpp -o taskset-gen.o -l$(MYLIBRARIES) $(CFLAGS_NEW)

request-driven-test.o: request-driven-test.cpp request-driven-test.hpp config.hpp
	$(CC) -c request-driven-test.cpp -o request-driven-test.o -l$(MYLIBRARIES) $(CFLAGS_NEW)

job-driven-test.o: job-driven-test.cpp job-driven-test.hpp config.hpp
	$(CC) -c job-driven-test.cpp -o job-driven-test.o -l$(MYLIBRARIES) $(CFLAGS_NEW)

hybrid-test.o: hybrid-test.cpp hybrid-test.hpp config.hpp
	$(CC) -c hybrid-test.cpp -o hybrid-test.o -l$(MYLIBRARIES) $(CFLAGS_NEW)

hyperperiod.o: hyperperiod.cpp hyperperiod.hpp config.hpp
	$(CC) -c hyperperiod.cpp -o hyperperiod.o -l$(MYLIBRARIES) $(CFLAGS_NEW)

indirect-cis.o: indirect-cis.cpp indirect-cis.hpp config.hpp
	$(CC) -c indirect-cis.cpp -o indirect-cis.o -l$(MYLIBRARIES) $(CFLAGS_NEW)

# task_partitioning.o: task_partitioning.cpp task_partitioning.hpp config.hpp
# 	$(CC) -c task_partitioning.cpp -o task_partitioning.o -l$(MYLIBRARIES) $(CFLAGS_NEW)

uniprocessor_exp.o: uniprocessor_exp.cpp config.hpp
	$(CC) -c uniprocessor_exp.cpp -o uniprocessor_exp.o -l$(MYLIBRARIES) $(CFLAGS_NEW)

# mcprocessor_exp.o: mcprocessor_exp.cpp config.hpp
# 	$(CC) -c mcprocessor_exp.cpp -o mcprocessor_exp.o -l$(MYLIBRARIES) $(CFLAGS_NEW)

# mcprocessor_sched_exp.o: mcprocessor_sched_exp.cpp config.hpp
# 	$(CC) -c mcprocessor_sched_exp.cpp -o mcprocessor_sched_exp.o -l$(MYLIBRARIES) $(CFLAGS_NEW)

# paper_examples.o: paper_examples.cpp config.hpp
# 	$(CC) -c paper_examples.cpp -o paper_examples.o -l$(MYLIBRARIES) $(CFLAGS_NEW)

uniprocessor_exp: $(UNIPROC_OBJ)
	$(CC) $(UNIPROC_OBJ) -o uniprocessor_exp -l$(MYLIBRARIES) $(CFLAGS_NEW)

# mcprocessor_exp: $(MCPROC_OBJ)
# 	$(CC) $(MCPROC_OBJ) -o mcprocessor_exp -l$(MYLIBRARIES) $(CFLAGS_NEW)

# mcprocessor_sched_exp: $(MCPROC_SCHED_OBJ)
# 	$(CC) $(MCPROC_SCHED_OBJ) -o mcprocessor_sched_exp -l$(MYLIBRARIES) $(CFLAGS_NEW)

# paper_examples: $(PAPEREX_OBJ)
# 	$(CC) $(PAPEREX_OBJ) -o paper_examples -l$(MYLIBRARIES) $(CFLAGS_NEW)

clean:
	rm -f uniprocessor_exp paper_examples mcprocessor_exp mcprocessor_sched_exp *.o

