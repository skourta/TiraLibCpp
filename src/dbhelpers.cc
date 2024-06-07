#include <string>

#include <sqlite3.h>
#include <TiraLibCPP/utils.h>

static int callback(void *data, int argc, char **argv, char **azColName)
{
    int i;
    fprintf(stderr, "%s: ", (const char *)data);

    for (i = 0; i < argc; i++)
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    printf("\n");
    return 0;
}

int write_wrapper(std::string function_name)
{

    // read databse path from environment variable
    char *db_path = getenv("TIRAMISU_DB_PATH");
    if (db_path == NULL)
    {
        if (file_exists(function_name + "_wrapper.cpp"))
        {
            compile_wrapper(function_name);
            return 0;
        }

        std::cerr << "TIRAMISU_DB_PATH environment variable not set" << std::endl;
        return -1;
    }

    sqlite3 *DB;
    int exit = 0;
    exit = sqlite3_open(db_path, &DB);

    if (exit)
    {
        std::cerr << "Error open DB " << sqlite3_errmsg(DB) << std::endl;
        return -1;
    }
    else
    {
        std::string sql = "SELECT * FROM wrappers WHERE program_name = '" + function_name + "'";
        std::string wrapper_name = function_name + "_wrapper";
        // read the wrapper from the database
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(DB, sql.c_str(), -1, &stmt, NULL);
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW)
        {
            // get the wrapper
            const void *wrapper = sqlite3_column_blob(stmt, 1);
            // get the size of the wrapper
            int wrapper_size = sqlite3_column_bytes(stmt, 1);
            // write the wrapper to a file
            std::ofstream wrapper_file;
            wrapper_file.open(wrapper_name, std::ios::out | std::ios::binary);
            wrapper_file.write((const char *)wrapper, wrapper_size);
            wrapper_file.close();
            // make the wrapper executable
            std::string chmod_cmd = "chmod +x " + wrapper_name;
            int status = system(chmod_cmd.c_str());
            assert(status != 139 && "Segmentation Fault when trying to execute schedule");
        }
        else
        {
            std::cerr << "No wrapper found for " << function_name << std::endl;
            return -1;
        }
    }
    return 0;
}
