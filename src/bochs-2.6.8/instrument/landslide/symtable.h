/**
 * @file symtable.h
 * @brief simics goo for querying the symbol table
 * @author Ben Blum
 */

#ifndef __LS_SYMTABLE_H
#define __LS_SYMTABLE_H

#include "simulator.h"

symtable_t *get_symtable();
void set_symtable(symtable_t *symtable);
bool symtable_lookup(unsigned int eip, char **func, char **file, int *line);
unsigned int symtable_lookup_data(char *buf, unsigned int maxlen, unsigned int addr);
bool function_eip_offset(unsigned int eip, unsigned int *offset);
bool find_user_global_of_type(const char *type_name, unsigned int *size_result);

#endif
