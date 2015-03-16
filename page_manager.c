/*
 * Aaron Schraufnagel
 * TCSS422
 * HW4
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "page_manager.h"

// stores physical address space, virtual address space, page size, # of processes
static memory_config config;

// struct for page table entry -> valid, virtual page number, processID, position
struct pageTableEntry {
	unsigned char valid;					/* valid bit */
	unsigned int virtual_page_number;
	unsigned char processID;
	unsigned int position;              /* to determine the oldest page */
};

// array of pagetableentry struct
struct pageTableEntry * pageTable;

// global variable for total number of page table entries (for all processes)
unsigned int totalNumPTE;

// global variable for order (to find oldest if page fault and all pte are full)
unsigned int order;

// global variables to keep track of vpn_mask, offset_mask and shift
unsigned int vpn_mask;
unsigned int offset_mask;
unsigned char shift;

unsigned int get_vpn(unsigned int virtual_address);
unsigned int get_offset(unsigned int virtual_address);
unsigned int get_physical_address(unsigned int physical_page_number, unsigned int offset);

/*
After this function is called, the page tables of all simulated
processes are considered to be empty (no valid entries) and no physical pages hold any virtual pages.
 */
void initialize_page_manager(memory_config mc) {
	config = mc;

	/* Other initialization work here. */
	totalNumPTE = (pow(2, config.physical_address_space)) / config.page_size;
	// printf("totalNumPTE = %d", totalNumPTE);
	int i;
	order = 0;
	vpn_mask = 0;
	offset_mask = 0;
	shift = 0;

	// Allocate array for struct of page table entries (keep in mind # processes), initialize process to specific value, position to 0
	pageTable = malloc(sizeof(struct pageTableEntry) * totalNumPTE);

	// Loop thru array of structs to initialize
	for (i = 0; i < totalNumPTE; i++) {
		pageTable[i].valid = 0;
		pageTable[i].processID = -1;
		pageTable[i].position = 0;
		// printf("PTE %d = valid: %d, processID: %d, position: %d\n", i, pageTable[i].valid, pageTable[i].processID, pageTable[i].position);
	}

	// Bits in VPN: log2(virtual address space/page_size)
	unsigned int vpnBits = config.virtual_address_space - log2(config.page_size);

	// VPN Mask
	for (i = config.virtual_address_space; i > config.virtual_address_space - vpnBits; i--) {
		vpn_mask += pow(2, i - 1);
	}
	// printf("VPN Mask = %d\n", vpn_mask);

	// Offset Mask
	for (i = 0; i < config.virtual_address_space - vpnBits; i++) {
		offset_mask += pow(2, i);
	}
	// printf("Offset Mask = %d\n", offset_mask);

	// shift
	shift = config.virtual_address_space - vpnBits;
	// printf("Shift = %d\n", shift);
}

/*
This function returns a structure containing information about the internal operation of the memory
manager for that memory operation, including the virtual page number
accessed, the physical page number that virtual page maps to, the complete physical address the virtual
address maps to, and whether accessing that virtual page corresponded to a page fault (the page not
existing in physical memory before the memory operation).
 */
access_result access_memory(unsigned int pid, unsigned int virtual_address) {
	// virtual page number, physical page number, physical address, page fault
	access_result result;

	/* Do work here. */
	unsigned int offset;
	int i;
	result.page_fault = 1;
	order++;

	// perform calculations to get value of virtual page number (maybe helper method)
	result.virtual_page_number = get_vpn(virtual_address);
	// printf("VPN = %d\n", result.virtual_page_number);
	// perform calculations to get value of offset
	offset = get_offset(virtual_address);
	// printf("Offset = %d\n", offset);

	// check array of page table entries for virtual page number (based on process id)
	for (i = 0; i < totalNumPTE; i++) {
		// if page number exists(valid = 1)
		if (pageTable[i].valid == 1 && pageTable[i].processID == pid && pageTable[i].virtual_page_number == result.virtual_page_number) {
			// store value of i % page table size (per process) for physical page number in result
			result.physical_page_number = i;
			// store value of 0 for page fault in result
			result.page_fault = 0;
			// printf("page table vpn %d result vpn %d", pageTable[i].virtual_page_number, result.virtual_page_number);
			// break loop
			break;
		}
		// printf("PTE %d = valid: %d, processID: %d, position: %d\n", i, pageTable[i].valid, pageTable[i].processID, pageTable[i].position);
	}

	// if page fault = 0
	if (result.page_fault == 0) {
		// add calculated value of physical page number to offset value and store as physical address in result
		result.physical_address = get_physical_address(result.physical_page_number, offset);
	} else {
		char mapped = 0;
		// check array of page table entries for invalid pte
		for (i = 0; i < totalNumPTE; i++) {
			// if page_table[i].valid = 0;
			if (pageTable[i].valid == 0) {
				pageTable[i].valid = 1;
				pageTable[i].virtual_page_number = result.virtual_page_number;
				pageTable[i].position = order;
				pageTable[i].processID = pid;
				// store value of i for physical page number in result
				result.physical_page_number = i;
				mapped = 1;
				break;
			}
		}

		// if (mapped = 0)
		if (mapped == 0) {
			// int oldest = 1st entry of process in page table (keep in mind process ids)
			int oldest = 0;
			// check array of page table entries for position < oldest
			for (i = 0; i < totalNumPTE; i++) {
				// if page_table[i].position < page_table[oldest].position
				if (pageTable[i].processID == pid && pageTable[i].position <= pageTable[oldest].position) {
					oldest = i;
				}
			}
			pageTable[oldest].valid = 1;
			pageTable[oldest].virtual_page_number = result.virtual_page_number;
			pageTable[oldest].position = order;
			pageTable[oldest].processID = pid;
			// store value of i for physical page number in result
			result.physical_page_number = oldest;
		}

		// add calculated value of physical page number to offset value and store as physical address in result
		result.physical_address = get_physical_address(result.physical_page_number, offset);
	}
	return result;
}

/*
 * Return the decimal value of the virtual page number.
 */
unsigned int get_vpn(unsigned int virtual_address) {
	return (virtual_address & vpn_mask) >> shift;
}

/*
 * Return the decimal value of the offset.
 */
unsigned int get_offset(unsigned int virtual_address) {
	return virtual_address & offset_mask;
}

/*
 * Take in decimal values for physical page number and offset, then translate them to the actual
 * physical address.
 */
unsigned int get_physical_address(unsigned int physical_page_number, unsigned int offset) {
	// printf("PPN = %d\n", physical_page_number);
	// printf("shift = %d\n", shift);
	return (physical_page_number << shift) | offset;
}
