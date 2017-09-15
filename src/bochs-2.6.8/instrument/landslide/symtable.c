/**
 * @file symtable.c
 * @brief querying the symbol table
 * @author Ben Blum
 */

#define MODULE_NAME "symtable glue"

#include "common.h"
#include "kspec.h"
#include "stack.h"
#include "symtable.h"
#include "x86.h"

#define SYMTABLE_NAME "deflsym"
#define CONTEXT_NAME "system.cell_context"

#define LIKELY_DIR "pebsim/"
#define UNKNOWN_FILE "410kern/boot/head.S"

#define GLOBAL_COLOUR        COLOUR_BOLD COLOUR_YELLOW
#define GLOBAL_INFO_COLOUR   COLOUR_DARK COLOUR_GREY

#ifdef BOCHS

symtable_t *get_symtable()
{
	return NULL;
}

void set_symtable(const symtable_t *symtable)
{
}

/* New interface. Returns malloced strings through output parameters,
 * which caller must free result strings if returnval is true */
bool symtable_lookup(unsigned int eip, char **func, char **file, int *line)
{
	return false;
}

unsigned int symtable_lookup_data(char *buf, unsigned int maxlen, unsigned int addr)
{
	return scnprintf(buf, maxlen, GLOBAL_COLOUR "unknown0x%.8x"
			 COLOUR_DEFAULT, addr);
}

/* Finds how many instructions away the given eip is from the start of its
 * containing function. */
bool function_eip_offset(unsigned int eip, unsigned int *offset)
{
#ifdef PINTOS_KERNEL
	// FIXME: hack to allow within_fns to properly find a cli PP's callsite
	// just remove this when symtable is properly implemented
	if (eip == GUEST_CLI_ENTER || eip == GUEST_SEMA_DOWN_ENTER) {
		*offset = 0;
		return true;
	}
#endif
	return false;
}

char *get_global_name_at(symtable_t *table, unsigned int addr, const char *type_name)
{
	return NULL;
}

#else
#include "symtable-simics.c"
#endif

/* Attempts to find an object of the given type in the global data region, and
 * learn its size. Writes to 'result' and returns true if successful. */
bool find_user_global_of_type(const char *type_name, unsigned int *size_result)
{
#if defined(USER_DATA_START) && defined(USER_IMG_END)
	symtable_t *symtable = get_symtable();
	if (symtable == NULL) {
		return false;
	}
	// Look for a global object of type mutex_t in the symtable.
	unsigned int start_addr = USER_DATA_START;
	unsigned int last_addr = USER_IMG_END;
	unsigned int addr, end_addr;
	char *name = NULL;
	assert(start_addr % WORD_SIZE == 0);
	assert(last_addr % WORD_SIZE == 0);

	for (addr = start_addr; addr < last_addr; addr += WORD_SIZE) {
		if ((name = get_global_name_at(symtable, addr, type_name)) != NULL) {
			break;
		}
	}
	if (addr == last_addr) {
		assert(name == NULL);
		return false;
	} else {
		assert(name != NULL);
	}
	// Ok, found a global mutex starting at addr.
	for (end_addr = addr + WORD_SIZE; end_addr < last_addr; end_addr += WORD_SIZE) {
		char *name2 = get_global_name_at(symtable, end_addr, type_name);
		bool same_name = false;
		if (name2 != NULL) {
			same_name = strncmp(name2, name, strlen(name2)+1) == 0;
			MM_FREE(name2);
		}
		if (!same_name) {
			// Found end of our mutex.
			break;
		}
	}
	// Victory.
	lsprintf(DEV, "Found a %s named %s of size %d (%x to %x)\n",
		 type_name, name, end_addr-addr, addr, end_addr);
	MM_FREE(name);
	*size_result = end_addr-addr;
	return true;
#else
	return false;
#endif
}
