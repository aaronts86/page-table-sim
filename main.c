#include <stdio.h>
#include <stdlib.h>
#include "page_manager.h"

struct access {
	char done;
	unsigned int pid;
	unsigned int virtual_address;
};

struct access access1();
struct access access2();
struct access access3();
struct access access4();

static struct sim {
	memory_config config;
	struct access (*access_func)();
} simulations[] = {{{12, 16, 512, 1}, access1},
                   {{14, 10, 128, 1}, access2},
                   {{10, 16, 64, 2}, access3},
                   {{24, 24, 2048, 2}, access4}};

#define FIRST_SIMULATION_TO_RUN 0
#define LAST_SIMULATION_TO_RUN 0

void run_simulation(struct sim simulation) {
	printf("Running simulation for the following memory configuration...\n");
	printf("Physical address space: %u bits\n", simulation.config.physical_address_space);
	printf(" Virtual address space: %u bits\n", simulation.config.virtual_address_space);
	printf("             Page size: %u bytes\n", simulation.config.page_size);
	printf("             Processes: %u\n", simulation.config.processes);
	printf("(pid, virt addr) -->  phys addr\n");

	initialize_page_manager(simulation.config);
	while (1) {
		struct access acc = simulation.access_func();
		if (acc.done)
			break;
		access_result result = access_memory(acc.pid, acc.virtual_address);
		printf("(%u, %11u) --> %10u  %s\n", acc.pid, acc.virtual_address, result.physical_address, result.page_fault ? "(page fault)" : "");
	}
}

int main(int argc, char* argv[]) {
	int i;
	for (i = FIRST_SIMULATION_TO_RUN; i <= LAST_SIMULATION_TO_RUN; i++)
		run_simulation(simulations[i]);

	return EXIT_SUCCESS;
}

struct access access1() {
	static int i = 0;
	return (struct access){i == 96, 0, i++ * 128};
}

struct access access2() {
	static int i = 0;
	return (struct access){i == 32, 0, i++ * 320 % 1024};
}

struct access access3() {
	static int i = 0;
	struct access acc = {i == 32, i / 2 % 2, (i / 4 * 2 + i % 2) * 64 - i % 4 / 3};
	i++;
	return acc;
}

struct access access4() {
	static int i = 0;
	struct access acc = {i == 16512, i % 2, i * (i % 2 + 1) * 512 % 16777216};
	i++;
	return acc;
}
