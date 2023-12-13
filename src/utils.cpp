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
#include <HermesII/utils.h>
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

std::tuple<bool, std::string> exec(const char *cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    // std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    auto pipe = popen(cmd, "r");
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        result += buffer.data();
    }
    auto rc = pclose(pipe);
    return std::make_tuple(rc == 0, result);
}

bool apply_action(std::string action_str, tiramisu::function *implicit_function, Result &result)
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
        prepare_schedules_for_legality_checks(true);
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
        prepare_schedules_for_legality_checks(true);
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
        std::string regex_str = "S\\(L(\\d),L(\\d),(-?\\d+),(-?\\d+),comps=\\[([\\w', ]*)\\]\\)";
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
        if (factor1 == 0 && factor2 == 0)
        {
            // Get the factors from the skewing solver
            auto auto_skewing_result = implicit_function->skewing_local_solver(comps, level1, level2, 1);

            // check outer parallelism factors
            auto outer_factors = std::get<0>(auto_skewing_result);
            // check if it's not empty
            if (outer_factors.size() > 0)
            {
                factor1 = outer_factors.at(0).first;
                factor2 = outer_factors.at(0).second;
            }
            else
            {
                // check the inner parallelism factors
                auto inner_factors = std::get<1>(auto_skewing_result);
                // check if it's not empty
                if (inner_factors.size() > 0)
                {
                    factor1 = inner_factors.at(0).first;
                    factor2 = inner_factors.at(0).second;
                }
                else
                {
                    // no factors found
                    is_legal = false;
                }
            }
        }
        if (is_legal)
        {
            result.additional_info = "skewing_factors:" + std::to_string(factor1) + "," + std::to_string(factor2);
            for (auto comp : comps)
            {
                comp->skew(level1, level2, factor1, factor2);
            }
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

        prepare_schedules_for_legality_checks(true);
        std::vector<int> levels = {};
        // push levels starting from 0 to level
        for (int i = 0; i <= level; i++)
        {
            levels.push_back(i);
        }
        std::vector<std::tuple<tiramisu::var, int>> factors = tiramisu::global::get_implicit_function()->correcting_loop_fusion_with_shifting({comps[0]}, *comps[1], levels);
        for (const auto &tuple : factors)
        {
            tiramisu::var var = std::get<0>(tuple);
            int value = std::get<1>(tuple);

            if (value != 0)
            {
                comps[1]->shift(var, value);
            }
        }

        is_legal &= factors.size() > 0;
        break;
    }
    case 'T':
    {
        if (action_str[1] == '1')
        {
            // std::cout << "Tiling 2D" << std::endl;
            // parse the string to get the level, the factor and the computations
            std::string regex_str = "T1\\(L(\\d),(\\d+),comps=\\[([\\w', ]*)\\]\\)";
            std::regex re(regex_str);
            std::smatch match;
            std::regex_search(action_str, match, re);
            int level1 = std::stoi(match[1]);
            int factor1 = std::stoi(match[2]);
            std::string comps_str = match[3];
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
                comp->tile(level1, factor1);
            }
            if (comps.size() > 1)
                implicit_function->fuse_comps_after_tiling(comps, 1);
        }
        else if (action_str[1] == '2')
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
            throw std::invalid_argument("Tiling only supports 1D, 2D, and 3D");
        }
        break;
    }

    default:
        std::cerr << "No action" << std::endl;
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

bool apply_actions_from_schedule_str(std::string schedule_str, tiramisu::function *implicit_function, Result &result)
{
    std::string delimiter = "|";
    size_t pos = 0;
    std::string token;
    bool is_legal = true;

    while ((pos = schedule_str.find(delimiter)) != std::string::npos)
    {
        token = schedule_str.substr(0, pos);
        // std::cout << token << std::endl;
        is_legal &= apply_action(token, implicit_function, result);
        schedule_str.erase(0, pos + delimiter.length());
    }

    // std::cout << schedule_str << std::endl;
    is_legal &= apply_action(schedule_str, implicit_function, result);

    return is_legal;
}

int compile_wrapper(std::string function_name)
{
    if (file_exists(function_name + "_wrapper.cpp"))
    {
        std::string compile_wrapper_command = "c++ -std=c++17 -fno-rtti -I${TIRAMISU_ROOT}/include -I${TIRAMISU_ROOT}/3rdParty/Halide/install/include -I${TIRAMISU_ROOT}/3rdParty/isl/include/ -I${TIRAMISU_ROOT}/benchmarks -L${TIRAMISU_ROOT}/build -L${TIRAMISU_ROOT}/3rdParty/Halide/install/lib64/ -L${TIRAMISU_ROOT}/3rdParty/isl/build/lib -o " + function_name + "_wrapper -ltiramisu -lHalide -ldl -lpthread -fopenmp -lm -Wl,-rpath,${TIRAMISU_ROOT}/build ./" + function_name + "_wrapper.cpp ./" + function_name + ".o.so -ltiramisu -lHalide -ldl -lpthread -fopenmp -lm -lisl";

        // run the command to compile the wrapper
        int status = system(compile_wrapper_command.c_str());
        assert(status != 139 && "Segmentation Fault when trying to execute schedule");
        return 0;
    }
    return -1;
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

Operation get_operation_from_string(std::string operation_str)
{
    if (operation_str == "legality")
        return Operation::legality;
    else if (operation_str == "execution")
        return Operation::execution;
    else if (operation_str == "annotations")
        return Operation::annotations;
    else
        assert(false && "Unknown operation");
}

std::string serialize_result(Result &result)
{
    std::string result_str = "{";
    result_str += "\"name\": \"" + result.name + "\",";
    result_str += "\"legality\": " + std::to_string(result.legality) + ",";
    result_str += "\"isl_ast\": \"" + result.isl_ast + "\",";
    result_str += "\"exec_times\": \"" + result.exec_times + "\",";
    result_str += "\"success\": " + std::to_string(result.success) + ",";
    result_str += "\"additional_info\": \"" + result.additional_info + "\"";
    result_str += "}";
    return result_str;
}

void schedule_str_to_result_str(std::string function_name, std::string schedule_str, Operation operation, std::vector<tiramisu::buffer *> buffers)
{
    if (operation == Operation::annotations)
    {

        auto ast = tiramisu::auto_scheduler::syntax_tree(tiramisu::global::get_implicit_function(), {});
        std::string program_json = tiramisu::auto_scheduler::evaluate_by_learning_model::get_program_json(ast);
        std::cout << program_json;
        return;
    }
    Result result = {
        .name = function_name,
        .legality = false,
        .exec_times = "",
        .additional_info = "",
        .success = true,
    };

    auto implicit_function = global::get_implicit_function();

    prepare_schedules_for_legality_checks();
    perform_full_dependency_analysis();
    bool is_legal = true;

    is_legal &= apply_actions_from_schedule_str(schedule_str, implicit_function, result);

    prepare_schedules_for_legality_checks();
    is_legal &= check_legality_of_function();
    result.legality = is_legal;
    implicit_function->gen_time_space_domain();
    implicit_function->gen_isl_ast();
    std::string isl_ast = implicit_function->generate_isl_ast_representation_string(nullptr, 0, "");
    result.isl_ast = isl_ast;

    if (is_legal && operation == Operation::execution)
    {
        tiramisu::codegen(buffers, function_name + ".o");

        std::string gpp_command = "g++";
        std::string wrapper_cmd = "./" + function_name + "_wrapper";

        std::string gcc_cmd = gpp_command + " -shared -o " + function_name + ".o.so " + function_name + ".o";
        // run the command and retrieve the execution status
        int status = system(gcc_cmd.c_str());
        assert(status != 139 && "Segmentation Fault when trying to execute schedule");
        // write the wrapper to a file if it does not exist
        if (!file_exists(function_name + "_wrapper"))
        {
            if (write_wrapper(function_name))
            {
                std::cout << "Error: could not write wrapper to file" << std::endl;
                // exit with error
                exit(1);
            };
        }
        // run the wrapper
        auto res_tuple = exec(wrapper_cmd.c_str());
        result.success = std::get<0>(res_tuple);
        result.exec_times = std::get<1>(res_tuple);
        // remove new line character
        if (!result.exec_times.empty() && result.exec_times[result.exec_times.length() - 1] == '\n')
        {
            result.exec_times.erase(result.exec_times.length() - 1);
        }
    }
    std::cout << serialize_result(result) << std::endl;
}