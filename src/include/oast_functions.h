#ifndef OAST_FUNCTIONS_H
#define OAST_FUNCTIONS_H

#include "duckdb_extension.h"

// Register all OAST scalar functions
void RegisterOASTFunctions(duckdb_connection connection);

#endif // OAST_FUNCTIONS_H
