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

/* don't change the order or names or add/remove any fields without editing the
 * corresponding header autogen code in pebsim/build.sh */
struct line_number {
	unsigned int eip;
	const char *filename;
	unsigned int line;
};

struct line_number line_numbers[] = {
/* should be sorted */
#include "line_numbers.h"
};

symtable_t *get_symtable() { return NULL; }
void set_symtable(const symtable_t *symtable) { }

static bool line_number_lookup(unsigned int eip, struct line_number **result)
{
	int min = 0, max = ARRAY_SIZE(line_numbers) - 1;
	while (true) {
		if (max < min) {
			return false;
		} else {
			int i = (max + min) / 2;
			assert(i < ARRAY_SIZE(line_numbers) && i >= 0);
			if (line_numbers[i].eip == eip) {
				*result = &line_numbers[i];
				return true;
			} else if (line_numbers[i].eip < eip) {
				min = i + 1;
			} else {
				max = i - 1;
			}
		}
	}
}

/* New interface. Returns malloced strings through output parameters,
 * which caller must free result strings if returnval is true */
bool symtable_lookup(unsigned int eip, char **func, char **file, int *line)
{
	char *result;
	unsigned int _offset;
	if (bx_dbg_symbolic_address_landslide(eip, &result, &_offset)) {
		*func = MM_XSTRDUP(result);
		struct line_number *line_number;
		if (line_number_lookup(eip, &line_number)) {
			*file = MM_XSTRDUP(line_number->filename);
			*line = line_number->line;
		} else {
			*file = MM_XSTRDUP("file unknown");
			*line = 0;
		}
		return true;
	} else {
		return false;
	}
}

unsigned int symtable_lookup_data(char *buf, unsigned int maxlen, unsigned int addr)
{
	char *result;
	unsigned int offset;
	if (bx_dbg_symbolic_address_landslide(addr, &result, &offset)) {
		if (offset == 0) {
			return scnprintf(buf, maxlen, GLOBAL_COLOUR "%s"
					 COLOUR_DEFAULT, result);
		} else {
			return scnprintf(buf, maxlen, GLOBAL_COLOUR "%s+0x%x"
					 COLOUR_DEFAULT, result, offset);
		}
	} else {
		return scnprintf(buf, maxlen, GLOBAL_COLOUR "unknown0x%.8x"
				 COLOUR_DEFAULT, addr);
	}
}

/* Finds how many instructions away the given eip is from the start of its
 * containing function. */
bool function_eip_offset(unsigned int eip, unsigned int *offset)
{
	char *_result;
	return bx_dbg_symbolic_address_landslide(eip, &_result, offset);
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
