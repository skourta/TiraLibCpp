import pickle
import re
from pathlib import Path
from typing import Dict, List, Tuple

from tqdm import tqdm

template = """
Result {name}(std::string schedule_str, Operation operation)
{{
    std::string function_name = "{name}";
    
    {body}

    Result result = {{
        .name = function_name,
        .legality = false,
        .exec_times = "",
    }};

    auto implicit_function = global::get_implicit_function();

    prepare_schedules_for_legality_checks();
    perform_full_dependency_analysis();
    bool is_legal = true;

    is_legal &= apply_actions_from_schedule_str(schedule_str, implicit_function);

    prepare_schedules_for_legality_checks();
    is_legal &= check_legality_of_function();
    result.legality = is_legal;

    std::string isl_ast = implicit_function->generate_isl_ast_representation_string(nullptr, 0, "");
    result.isl_ast = isl_ast;

    if (operation == Operation::execution)
    {{
        {code_gen_line}

        std::string gpp_command = "g++";
        std::string wrapper_cmd = "./" + function_name + "_wrapper";

        std::string gcc_cmd = gpp_command + " -shared -o " + function_name + ".o.so " + function_name + ".o";
        // run the command and retrieve the execution status
        int status = system(gcc_cmd.c_str());
        assert(status != 139 && "Segmentation Fault when trying to execute schedule");
        // write the wrapper to a file
        write_wrapper_from_db(function_name);
        // run the wrapper
        result.exec_times = exec(wrapper_cmd.c_str());
    }}
    result.success = true;
    return result;
}}
"""


switchTemplate = """
Result switchFunction(std::string function_name, std::string schedule, Operation operation)
{{
    Result res = {{
        .name = function_name,
        .legality = false,
        .exec_times = "",
        .success = false,
    }};
    {body}
    else
    {{
        std::cout << "Function " + function_name + " not supported" << std::endl;
    }}
    return res;
}}
"""

switchIfTemplate = """
    if (function_name.compare("{name}") == 0)
    {{
        res = {name}(schedule, operation);
    }}
"""

mapTemplate = """

#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include <hermes_ii/utils.h>
#include <hermes_ii/functions.h>

using namespace tiramisu;

std::map<std::string, std::function<Result(std::string, Operation)>> createFuncMap()
{{
    std::map<std::string, std::function<Result(std::string, Operation)>> funcMap;
{body}
    return funcMap;
}}

"""

mapFunctionTemplate = """    funcMap["{name}"] = {name};
"""


includesFunctionsCpp = """
#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include <hermes_ii/utils.h>

using namespace tiramisu;

"""

functionsHeaderTemplate = """
#pragma once

#include <hermes_ii/utils.h>
"""

cmakeHeaderTemplate = """
# project(HermesII)

set(INCLUDES
${HalideInclude}
${TIRAMISU_ROOT}/include/
${TIRAMISU_ROOT}/3rdParty/isl/include
${ENV_INCLUDES}
${PROJECT_SOURCE_DIR}/include
)

"""

cmakeFunctionTemplate = """

add_library(functions{i} functions{i}.cpp)
target_include_directories(functions{i} PUBLIC ${{INCLUDES}})
target_link_directories(functions{i} PUBLIC ${{TIRAMISU_ROOT}}/build ${{HalideLib}} ${{TIRAMISU_ROOT}}/3rdParty/isl/build/lib)
target_link_libraries(functions{i} tiramisu tiramisu_auto_scheduler Halide isl)

"""

cmakeSwitchTemplate = """

add_library(switch{i} switchFunction{i}.cpp)
target_include_directories(switch{i} PUBLIC ${{INCLUDES}})
target_link_directories(switch{i} PUBLIC ${{TIRAMISU_ROOT}}/build ${{HalideLib}} ${{TIRAMISU_ROOT}}/3rdParty/isl/build/lib)
target_link_libraries(switch{i} {funcs} tiramisu tiramisu_auto_scheduler Halide isl)

"""

NUMBER_FILES = 300


def generate_function_from_cpp_file(original_str: str):
    """
    Generate a function from a cpp file

    Args:
        cpp_code: the code of the cpp file

    Returns:
        the generated function code
    """
    # Generate function
    body = re.findall(r"(tiramisu::init(?s:.)+)tiramisu::codegen", original_str)[0]
    name = re.findall(r"tiramisu::init\(\"(\w+)\"\);", original_str)[0]
    # Remove the wrapper include from the original string
    wrapper_str = f'#include "{name}_wrapper.h"'
    original_str = original_str.replace(wrapper_str, f"// {wrapper_str}")
    code_gen_line = re.findall(r"tiramisu::codegen\({.+;", original_str)[0]

    # fill the template
    function_str = template.format(name=name, body=body, code_gen_line=code_gen_line)
    return function_str


def generateSwitchFunction(function_names: List[str], key: str = ""):
    assert len(function_names) > 0
    switchTemplate = """
Result switchFunction{key}(std::string function_name, std::string schedule, Operation operation)
{{
    Result res = {{
        .name = function_name,
        .legality = false,
        .exec_times = "",
        .success = false,
    }};
    {body}
    else
    {{
        std::cout << "Function " + function_name + " not supported" << std::endl;
    }}
    return res;
}}
"""
    body = ""
    body += switchIfTemplate.format(name=function_names[0])
    for name in function_names[1:]:
        body += "else " + switchIfTemplate.format(name=name)
    switch_str = switchTemplate.format(key=key, body=body)
    return switch_str


def generateSwitchFunctions(
    function_names: List[str], dest_dir: str = "./src/switches"
):
    switchHeaders = """

#include <hermes_ii/utils.h>
#include <hermes_ii/functions.h>
"""
    moddict = {}
    switchCmakeContent = cmakeHeaderTemplate
    switchHeadersContent = functionsHeaderTemplate
    assert len(function_names) > 0

    nbr_numbers = len(str(len(functions_names) // NUMBER_FILES))
    # nbr_numbers = 3
    funclibs = set()
    for i in range(len(functions_names)):
        num = str(i // NUMBER_FILES).zfill(nbr_numbers)
        funclibs.add(f"functions{num}")
    print(funclibs)
    funclibs = " ".join(sorted(funclibs))

    for name in function_names:
        # get function number
        num = int(name[8:])
        # get the id
        rem = num % 100
        # add the function to the dict
        funclist = moddict.setdefault(rem, [])
        funclist.append(name)

    # generate the switch functions
    for key, value in moddict.items():
        switch_str = generateSwitchFunction(value, key)
        # write the content to a file
        with open(Path(dest_dir) / f"switchFunction{key}.cpp", "w") as f:
            f.write(switchHeaders + switch_str)

        switchCmakeContent += cmakeSwitchTemplate.format(i=key, funcs=funclibs)
        switchHeadersContent += f"Result switchFunction{key}(std::string function_name, std::string schedule, Operation operation);\n"

    print(" ".join(sorted([f"switch{i}" for i in moddict.keys()])))

    # write cmake file
    with open(Path(dest_dir) / "CMakeLists.txt", "w") as f:
        f.write(switchCmakeContent)

    # write header file
    with open(Path("./include/hermes_ii/switches.h"), "w") as f:
        f.write(switchHeadersContent)

    # generate the map function
    map_str = generateSwitchMap(moddict.keys())
    with open(Path(dest_dir) / "../switchMap.cpp", "w") as f:
        f.write(map_str)


def generateSwitchMap(keys: List[str]):
    templateSwitchMap = """

#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include <hermes_ii/utils.h>
#include <hermes_ii/functions.h>
#include <hermes_ii/switches.h>

using namespace tiramisu;

std::map<int, std::function<Result(std::string, std::string, Operation)>> createSwitchMap()
{{
    std::map<int, std::function<Result(std::string, std::string, Operation)>> switchMap;

    {body}

    return switchMap;
}}

"""
    body = ""
    for key in keys:
        body += f"    switchMap[{key}] = switchFunction{key};\n"

    return templateSwitchMap.format(body=body)


def generateMapFunction(function_names: List[str]):
    body = ""
    for name in function_names:
        body += mapFunctionTemplate.format(name=name)

    map_str = mapTemplate.format(body=body)
    return map_str


def generate_functions_cpp_file(functions: List[Tuple[str, str]]):
    content = includesFunctionsCpp

    # generate functions
    for name, cpp_code in functions:
        function_str = generate_function_from_cpp_file(cpp_code)
        content += function_str + "\n\n"

    # generate switch function

    function_names = [name for name, _ in functions]
    switch_str = generateSwitchFunction(function_names)
    content += switch_str

    return content


def generate_functions_cpp_files(
    functions: List[Tuple[str, str]], dest_dir: str = "./src/functions"
):
    i = 0
    content = includesFunctionsCpp
    cmakeContent = cmakeHeaderTemplate
    # generate functions
    nbr_numbers = len(str(len(functions) // NUMBER_FILES))
    for name, cpp_code in tqdm(functions):
        function_str = generate_function_from_cpp_file(cpp_code)
        content += function_str + "\n\n"

        # write the content to a file every FUNCTIONS_PER_FILE functions
        if (i + 1) % NUMBER_FILES == 0:
            num = str(i // NUMBER_FILES).zfill(nbr_numbers)
            with open(
                Path(dest_dir) / f"functions{num}.cpp",
                "w",
            ) as f:
                f.write(content)

            cmakeContent += cmakeFunctionTemplate.format(i=num)
            content = includesFunctionsCpp
        i += 1

    # write cmake file
    with open(Path(dest_dir) / "CMakeLists.txt", "w") as f:
        f.write(cmakeContent)

    # generate switch function
    # content = includesFunctionsCpp
    # content += "#include <hermes_ii/functions.h> \n\n"
    # function_names = [name for name, _ in functions]
    # switch_str = generateSwitchFunction(function_names)
    # content += switch_str

    # # write the content to a file
    # with open(Path(dest_dir) / "../switchFunction.cpp", "w") as f:
    #     f.write(content)


def generate_functions_and_switches_files(
    functions: Dict[str, str], dest_dir: str = "./src/functions"
):
    i = 0

    cmakeContent = cmakeHeaderTemplate
    contentFunctionsHeaderFile = functionsHeaderTemplate
    # generate functions
    # nbr_numbers = len(str(len(functions) // NUMBER_FILES))
    switchCmakeContent = cmakeHeaderTemplate
    switchHeadersContent = """
#include <hermes_ii/utils.h>
#include <hermes_ii/functions.h>
"""
    hashTable = {}
    for name in functions:
        # get function number
        num = int(name[8:])
        # get the hash
        hash = num % NUMBER_FILES
        hash = str(hash).zfill(3)

        # add the function to the dict
        funclist = hashTable.setdefault(hash, [])
        funclist.append(name)

    # generate functions
    nbr_numbers = len(str(NUMBER_FILES))

    for key in tqdm(sorted(hashTable.keys())):
        value = hashTable[key]
        value.sort()
        content = includesFunctionsCpp
        for name in value:
            function_str = generate_function_from_cpp_file(functions[name])
            content += function_str + "\n\n"
            contentFunctionsHeaderFile += (
                f"Result {name}(std::string schedule_str, Operation operation);\n"
            )

        # write the content to a file every FUNCTIONS_PER_FILE functions
        with open(
            Path(dest_dir) / f"functions{key}.cpp",
            "w",
        ) as f:
            f.write(content)

        cmakeContent += cmakeFunctionTemplate.format(i=key)

        # generate switch function
        switch_str = generateSwitchFunction(value, key)
        # write the content to a file
        with open(Path(dest_dir) / f"../switches/switchFunction{key}.cpp", "w") as f:
            f.write(switchHeadersContent + switch_str)

        switchCmakeContent += cmakeSwitchTemplate.format(i=key, funcs=f"functions{key}")
        switchHeadersContent += f"Result switchFunction{key}(std::string function_name, std::string schedule, Operation operation);\n"

    # write cmake file
    with open(Path(dest_dir) / "CMakeLists.txt", "w") as f:
        f.write(cmakeContent)

    # generate functions header file
    with open(Path("./include/hermes_ii/functions.h"), "w") as f:
        f.write(contentFunctionsHeaderFile)

    # write cmake file
    with open(Path(dest_dir) / "../switches/CMakeLists.txt", "w") as f:
        f.write(switchCmakeContent)

    # write header file
    with open(Path("./include/hermes_ii/switches.h"), "w") as f:
        f.write(switchHeadersContent)

    # generate the map function
    map_str = generateSwitchMap(sorted(hashTable.keys()))
    with open(Path(dest_dir) / "../switchMap.cpp", "w") as f:
        f.write(map_str)


def generate_functions_header_file(functions: List[str]):
    contentFunctionsHeaderFile = functionsHeaderTemplate

    # generate functions
    for name in functions:
        contentFunctionsHeaderFile += (
            f"Result {name}(std::string schedule_str, Operation operation);\n"
        )

    return contentFunctionsHeaderFile


# def generate_cmake_file(functions)
if __name__ == "__main__":
    pkl_path = "/scratch/sk10691/workspace/dataset_source/merged_cpps.pkl"

    with open(pkl_path, "rb") as f:
        cpps = pickle.load(f)

    functions_names = list(cpps.keys())
    # print(functions_names)

    functions_tuples = [(name, cpps[name]) for name in functions_names]

    # content = generate_functions_cpp_files(functions_tuples)

    # with open("functions.cpp", "w") as f:
    #     f.write(content)

    # content = generate_functions_header_file(functions_names)

    # with open("./include/hermes_ii/functions.h", "w") as f:
    #     f.write(content)

    # content = generateMapFunction(functions_names)

    # with open("./src/functions_map.cpp", "w") as f:
    #     f.write(content)

    # generate switch function
    # generateSwitchFunctions(functions_names)
    # content = includesFunctionsCpp
    # content += "#include <hermes_ii/functions.h> \n\n"
    # switch_str = generateSwitchFunction(functions_names)
    # content += switch_str

    # write the content to a file
    # with open(Path("./src/functions") / "../switchFunction.cpp", "w") as f:
    #     f.write(content)

    generate_functions_and_switches_files(cpps)
