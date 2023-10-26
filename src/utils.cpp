#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include <chrono>
#include <sqlite3.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <hermes_ii/utils.h>
// #include "function_floyd_warshall_MINI_wrapper.h"

using namespace tiramisu;

bool isSingleQuoteOrWhiteSpace(char c)
{
    return (c == '\'' || std::isspace(c));
}
inline bool file_exists(const std::string &name)
{
    if (FILE *file = fopen(name.c_str(), "r"))
    {
        fclose(file);
        return true;
    }
    else
    {
        return false;
    }
}
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

std::string exec(const char *cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return result;
}

bool apply_action(std::string action_str, tiramisu::function *implicit_function)
{
    bool is_legal = true;
    switch (action_str[0])
    {
    case 'P':
    {
        // std::cout << "Parallelize" << std::endl;
        // parse the string to get the level and the computations
        std::string regex_str = "P\\(L(\\d),comps=\\[([\\w', ]*)\\]\\)";
        std::regex re(regex_str);
        std::smatch match;
        std::regex_search(action_str, match, re);
        int level = std::stoi(match[1]);
        std::string comps_str = match[2];
        comps_str.erase(std::remove_if(comps_str.begin(), comps_str.end(), isSingleQuoteOrWhiteSpace), comps_str.end());
        // std::cout << "level: " << level << std::endl;
        // std::cout << "comps: " << comps_str << std::endl;
        // first_comp = first_comp.substr(1, first_comp.length() - 2);
        // std::cout << "first_comp: " << first_comp << std::endl;
        // get the computation
        auto comps = get_comps(comps_str, implicit_function);

        // if (operation == Operation::legality)
        is_legal = loop_parallelization_is_legal(level, comps);

        comps[0]->tag_parallel_level(level);
        break;
    }
    case 'U':
    {
        // std::cout << "Unroll" << std::endl;
        // parse the string to get the level, the factor and the computations
        std::string regex_str = "U\\(L(\\d),(\\d+),comps=\\[([\\w', ]*)\\]\\)";
        std::regex re(regex_str);
        std::smatch match;
        std::regex_search(action_str, match, re);
        int level = std::stoi(match[1]);
        int factor = std::stoi(match[2]);
        std::string comps_str = match[3];
        comps_str.erase(std::remove_if(comps_str.begin(), comps_str.end(), isSingleQuoteOrWhiteSpace), comps_str.end());
        // std::cout << "level: " << level << std::endl;
        // std::cout << "factor: " << factor << std::endl;
        // std::cout << "comps: " << comps_str << std::endl;
        auto comps = get_comps(comps_str, implicit_function);

        // if (operation == Operation::legality)
        is_legal = loop_unrolling_is_legal(level, comps);

        for (auto comp : comps)
        {
            comp->unroll(level, factor);
        }
        break;
    }

    case 'I':
    {
        // std::cout << "Interchange" << std::endl;
        // parse the string to get the level, the factor and the computations
        std::string regex_str = "I\\(L(\\d),L(\\d),comps=\\[([\\w', ]*)\\]\\)";
        std::regex re(regex_str);
        std::smatch match;
        std::regex_search(action_str, match, re);
        int level1 = std::stoi(match[1]);
        int level2 = std::stoi(match[2]);
        std::string comps_str = match[3];
        comps_str.erase(std::remove_if(comps_str.begin(), comps_str.end(), isSingleQuoteOrWhiteSpace), comps_str.end());
        // std::cout << "level1: " << level1 << std::endl;
        // std::cout << "level2: " << level2 << std::endl;
        // std::cout << "comps: " << comps_str << std::endl;
        auto comps = get_comps(comps_str, implicit_function);
        for (auto comp : comps)
        {
            comp->interchange(level1, level2);
        }
        break;
    }
    case 'R':
    {
        // std::cout << "Reverse" << std::endl;
        // parse the string to get the level and the computations
        std::string regex_str = "R\\(L(\\d),comps=\\[([\\w', ]*)\\]\\)";
        std::regex re(regex_str);
        std::smatch match;
        std::regex_search(action_str, match, re);
        int level = std::stoi(match[1]);
        std::string comps_str = match[2];
        comps_str.erase(std::remove_if(comps_str.begin(), comps_str.end(), isSingleQuoteOrWhiteSpace), comps_str.end());
        // std::cout << "level: " << level << std::endl;
        // std::cout << "comps: " << comps_str << std::endl;
        auto comps = get_comps(comps_str, implicit_function);
        for (auto comp : comps)
        {
            comp->loop_reversal(level);
        }
        break;
    }
    case 'S':
    {
        // std::cout << "Skewing" << std::endl;
        // parse the string to get the level, the factor and the computations
        std::string regex_str = "S\\(L(\\d),L(\\d),(\\d+),(\\d+),comps=\\[([\\w', ]*)\\]\\)";
        std::regex re(regex_str);
        std::smatch match;
        std::regex_search(action_str, match, re);
        int level1 = std::stoi(match[1]);
        int level2 = std::stoi(match[2]);
        int factor1 = std::stoi(match[3]);
        int factor2 = std::stoi(match[4]);
        std::string comps_str = match[5];
        comps_str.erase(std::remove_if(comps_str.begin(), comps_str.end(), isSingleQuoteOrWhiteSpace), comps_str.end());
        // std::cout << "level1: " << level1 << std::endl;
        // std::cout << "level2: " << level2 << std::endl;
        // std::cout << "factor1: " << factor1 << std::endl;
        // std::cout << "factor2: " << factor2 << std::endl;
        // std::cout << "comps: " << comps_str << std::endl;
        auto comps = get_comps(comps_str, implicit_function);
        for (auto comp : comps)
        {
            comp->skew(level1, level2, factor1, factor2);
        }
        break;
    }
    case 'F':
    {
        // std::cout << "Fusion" << std::endl;
        // parse the string to get the level, and the computations
        std::string regex_str = "F\\(L(\\d),comps=\\[([\\w', ]*)\\]\\)";
        std::regex re(regex_str);
        std::smatch match;
        std::regex_search(action_str, match, re);
        int level = std::stoi(match[1]);
        std::string comps_str = match[2];
        comps_str.erase(std::remove_if(comps_str.begin(), comps_str.end(), isSingleQuoteOrWhiteSpace), comps_str.end());
        // std::cout << "level: " << level << std::endl;
        // std::cout << "comps: " << comps_str << std::endl;
        auto comps = get_comps(comps_str, implicit_function);
        // only accept fusion of two computations
        assert(comps.size() == 2);
        implicit_function->fuse_comps_sched_graph(comps[0], comps[1], level);
        break;
    }
    case 'T':
    {
        if (action_str[1] == '2')
        {
            // std::cout << "Tiling 2D" << std::endl;
            // parse the string to get the level, the factor and the computations
            std::string regex_str = "T2\\(L(\\d),L(\\d),(\\d+),(\\d+),comps=\\[([\\w', ]*)\\]\\)";
            std::regex re(regex_str);
            std::smatch match;
            std::regex_search(action_str, match, re);
            int level1 = std::stoi(match[1]);
            int level2 = std::stoi(match[2]);
            int factor1 = std::stoi(match[3]);
            int factor2 = std::stoi(match[4]);
            std::string comps_str = match[5];
            comps_str.erase(std::remove_if(comps_str.begin(), comps_str.end(), isSingleQuoteOrWhiteSpace), comps_str.end());
            // std::cout << "level1: " << level1 << std::endl;
            // std::cout << "level2: " << level2 << std::endl;
            // std::cout << "factor1: " << factor1 << std::endl;
            // std::cout << "factor2: " << factor2 << std::endl;
            // std::cout << "comps: " << comps_str << std::endl;
            // COMPS NEED TO BE ORDERED BY APPEARANCE
            auto comps = get_comps(comps_str, implicit_function);
            for (auto comp : comps)
            {
                comp->tile(level1, level2, factor1, factor2);
            }
            if (comps.size() > 1)
                implicit_function->fuse_comps_after_tiling(comps, 2);
        }
        else if (action_str[1] == '3')
        {
            // std::cout << "Tiling 3D" << std::endl;
            // parse the string to get the level, the factor and the computations
            std::string regex_str = "T3\\(L(\\d),L(\\d),L(\\d),(\\d+),(\\d+),(\\d+),comps=\\[([\\w', ]*)\\]\\)";
            std::regex re(regex_str);
            std::smatch match;
            std::regex_search(action_str, match, re);
            int level1 = std::stoi(match[1]);
            int level2 = std::stoi(match[2]);
            int level3 = std::stoi(match[3]);
            int factor1 = std::stoi(match[4]);
            int factor2 = std::stoi(match[5]);
            int factor3 = std::stoi(match[6]);
            std::string comps_str = match[7];
            comps_str.erase(std::remove_if(comps_str.begin(), comps_str.end(), isSingleQuoteOrWhiteSpace), comps_str.end());
            // std::cout << "level1: " << level1 << std::endl;
            // std::cout << "level2: " << level2 << std::endl;
            // std::cout << "level3: " << level3 << std::endl;
            // std::cout << "factor1: " << factor1 << std::endl;
            // std::cout << "factor2: " << factor2 << std::endl;
            // std::cout << "factor3: " << factor3 << std::endl;
            // std::cout << "comps: " << comps_str << std::endl;
            auto comps = get_comps(comps_str, implicit_function);
            for (auto comp : comps)
            {
                comp->tile(level1, level2, level3, factor1, factor2, factor3);
            }
            if (comps.size() > 1)
                implicit_function->fuse_comps_after_tiling(comps, 3);
        }
        else
        {
            throw std::invalid_argument("Tiling only supports 2D and 3D");
        }
        break;
    }

    default:
        std::cout << "No action" << std::endl;
        break;
    }

    return is_legal;
}

tiramisu::computation *get_computation_by_name(std::string comp_name, tiramisu::function *implicit_function)
{
    auto comps = implicit_function->get_computation_by_name(comp_name);
    if (comps.size() == 0)
    {
        // std::cout << "No computation with name " << first_comp << std::endl;
        // throw error
        throw std::invalid_argument("No computation with name " + comp_name);
        return nullptr;
    }
    else if (comps.size() > 1)
    {
        // std::cout << "More than one computation with name " << first_comp << std::endl;
        // throw error
        throw std::invalid_argument("More than one computation with name " + comp_name);
        return nullptr;
    }
    else
    {
        return comps[0];
    }
}

std::string get_first_comp(std::string comps_str)
{
    std::string delimiter = ",";
    size_t pos = 0;
    std::string token;
    pos = comps_str.find(delimiter);
    token = comps_str.substr(0, pos);
    return token;
}

std::vector<tiramisu::computation *> get_comps(std::string comps_str, tiramisu::function *implicit_function)
{
    std::vector<tiramisu::computation *> comps;
    std::string delimiter = ",";
    size_t pos = 0;
    std::string token;
    while ((pos = comps_str.find(delimiter)) != std::string::npos)
    {
        token = comps_str.substr(0, pos);
        comps.push_back(get_computation_by_name(token, implicit_function));
        comps_str.erase(0, pos + delimiter.length());
    }
    comps.push_back(get_computation_by_name(comps_str, implicit_function));
    return comps;
}

bool apply_actions_from_schedule_str(std::string schedule_str, tiramisu::function *implicit_function)
{
    std::string delimiter = "|";
    size_t pos = 0;
    std::string token;
    bool is_legal = true;

    while ((pos = schedule_str.find(delimiter)) != std::string::npos)
    {
        token = schedule_str.substr(0, pos);
        std::cout << token << std::endl;
        is_legal &= apply_action(token, implicit_function);
        schedule_str.erase(0, pos + delimiter.length());
    }

    std::cout << schedule_str << std::endl;
    is_legal &= apply_action(schedule_str, implicit_function);

    return is_legal;
}
int write_wrapper_from_db(std::string function_name)
{

    sqlite3 *DB;
    int exit = 0;
    exit = sqlite3_open("/scratch/sk10691/workspace/dataset_source/wrappers.db", &DB);

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
            std::cout << "No wrapper found for " << function_name << std::endl;
            return -1;
        }
    }

    return 0;
}