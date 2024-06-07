#pragma once

static int callback(void *data, int argc, char **argv, char **azColName);

int write_wrapper_from_db(std::string function_name);