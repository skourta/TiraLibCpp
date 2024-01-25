import argparse
import logging
import pickle
import random
import re
import subprocess
import time
from pathlib import Path
from shutil import rmtree
from typing import Dict, List, Tuple

from tqdm import tqdm

template = """
#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include <HermesII/utils.h>

using namespace tiramisu;

int main(int argc, char *argv[])
{{
    // check the number of arguemnts is 2 or 3
    assert(argc == 1 || argc == 2 || argc == 3 && "Invalid number of arguments");
    // get the operation to perform
    Operation operation = Operation::legality;

    if (argc >= 2)
    {{
        operation = get_operation_from_string(argv[1]);
    }}
    // get the schedule string if provided
    std::string schedule_str = "";
    if (argc == 3)
        schedule_str = argv[2];

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
        if (write_wrapper_from_db(function_name)){{
            std::cout << "Error: could not write wrapper to file" << std::endl;
            return 1;
        }};
        // run the wrapper
        result.exec_times = exec(wrapper_cmd.c_str());
    }}
    result.success = true;

    std::cout << serialize_result(result) << std::endl;
    return 0;
}}
"""

templateWithEverythinginHermesII = """
#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include <HermesII/utils.h>

using namespace tiramisu;

int main(int argc, char *argv[])
{{
    // check the number of arguemnts is 2 or 3
    assert(argc == 1 || argc == 2 || argc == 3 && "Invalid number of arguments");
    // get the operation to perform
    Operation operation = Operation::legality;

    if (argc >= 2)
    {{
        operation = get_operation_from_string(argv[1]);
    }}
    // get the schedule string if provided
    std::string schedule_str = "";
    if (argc == 3)
        schedule_str = argv[2];

    std::string function_name = "{name}";
    
    {body}

    schedule_str_to_result_str(function_name, schedule_str, operation, {buffers});
    return 0;
}}
"""
cmakeHeaderTemplate = """
set(INCLUDES
${HalideInclude}
${TIRAMISU_ROOT}/include/
${TIRAMISU_ROOT}/3rdParty/isl/include
${ENV_INCLUDES}
${PROJECT_SOURCE_DIR}/include
)

"""

cmakeFunctionTemplate = """

add_executable({function} {function}.cpp)
target_link_directories({function} PUBLIC ${{TIRAMISU_ROOT}}/build ${{HalideLib}} ${{TIRAMISU_ROOT}}/3rdParty/isl/build/lib {libHermesIIPath})
target_include_directories({function} PUBLIC ${{INCLUDES}})
target_link_libraries({function} HermesII tiramisu tiramisu_auto_scheduler Halide isl sqlite3 ZLIB::ZLIB)

"""


def generate_function_from_cpp_file(original_str: str, abstracted: bool = False):
    """
    Generate a function from a cpp file

    Args:
        cpp_code: the code of the cpp file

    Returns:
        the generated function code
    """
    print(original_str)
    # Generate function
    body = re.findall(
        r"int main\([\w\s,*]+\)\s*\{([\W\w\s]*)tiramisu::codegen", original_str
    )[0]
    name = re.findall(r"tiramisu::init\(\"(\w+)\"\);", original_str)[0]
    # Remove the wrapper include from the original string
    wrapper_str = f'#include "{name}_wrapper.h"'
    original_str = original_str.replace(wrapper_str, f"// {wrapper_str}")
    code_gen_line = re.findall(r"tiramisu::codegen\({.+;", original_str)[0]
    buffers_vector = re.findall(r"(?<=tiramisu::codegen\()\{[&\w,\s]+\}", original_str)[
        0
    ]

    # fill the template
    if abstracted:
        function_str = templateWithEverythinginHermesII.format(
            name=name,
            body=body,
            buffers=buffers_vector,
        )
    else:
        function_str = template.format(
            name=name, body=body, code_gen_line=code_gen_line
        )
    return function_str


def generate_functions(
    function_names: List[Tuple[str, str]],
    libHermesIIPath: str,
    dest_path: str = "./src/functions",
):
    cmakeContent = cmakeHeaderTemplate

    # for name, body in tqdm(function_names):
    for name, body in function_names:
        function_content = generate_function_from_cpp_file(body, True)
        function_path = Path(dest_path) / f"{name}.cpp"

        with open(function_path, "w") as f:
            f.write(function_content)

        cmakeContent += cmakeFunctionTemplate.format(
            function=name, libHermesIIPath=libHermesIIPath
        )

    cmakeContent += "\n"

    with open(Path(dest_path) / "CMakeLists.txt", "w") as f:
        f.write(cmakeContent)


# add get args function for the script
def get_args():
    parser = argparse.ArgumentParser(
        description="Generate functions from a pickle file"
    )
    parser.add_argument(
        "--pkl",
        type=str,
        help="Path to the pickle file containing the functions",
        # default="./tmp/merged_cpps.pkl",
        default="/scratch/sk10691/workspace/dataset_source/polybench_cpps.pkl",
    )
    parser.add_argument(
        "--dest",
        type=str,
        help="Path to the destination pickle file to store the generated functions",
        default="",
    )
    # batch size
    parser.add_argument("--batch-size", type=int, help="Batch size", default=28)

    # ID of job with a randomly generated default number
    parser.add_argument(
        "--job-id",
        type=int,
        help="Job ID",
        # default=random.randint(0, 1000000)
        default=1,
    )

    # saving frequency
    parser.add_argument(
        "--saving-frequency",
        type=int,
        help="Saving frequency",
        default=2,
    )

    # libHermesII path
    parser.add_argument(
        "--libHermesII-path",
        type=str,
        help="Path to the libHermesII.so file",
        default="/scratch/sk10691/workspace/grpc/server-tiramisu-grpc-2/tmp",
    )
    args = parser.parse_args()
    return args


def compile_functions():
    script = """
    rm -fr build && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j && \
    cd ..
    """
    # script = """
    # cd build && \
    # cmake .. && \
    # make -j && \
    # cd ..
    # """
    try:
        output = subprocess.check_output(script, shell=True, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        output = e.output
        stderr = e.stderr
        stdout = e.stdout
        logging.error("Error compiling functions")
        logging.error(output)
        logging.error(stderr)
        logging.error(stdout)
    logging.debug(output)


def clean_functions():
    rmtree("./src/functions")
    Path("./src/functions").mkdir(parents=True, exist_ok=True)


def store_generated_functions(dest: Dict["str", bytes]):
    build_path = Path("./build/src/functions")

    # starts with function
    for path in build_path.glob("function*"):
        name = path.name
        with open(path, "rb") as f:
            dest[str(name)] = f.read()


# def generate_cmake_file(functions)
if __name__ == "__main__":
    args = get_args()

    # start logging
    logging.basicConfig(
        filename=f"outputs/generate_code{args.job_id}.log",
        level=logging.DEBUG,
        format="%(asctime)s - %(levelname)s - %(message)s",
    )

    logging.info(args.__dict__)

    with open(args.pkl, "rb") as f:
        cpps = pickle.load(f)

    generated_functions = {}

    # load the functions that have already been generated
    if args.dest == "":
        path_dest = Path(args.pkl)
        path_dest_0 = Path(args.pkl).with_name(
            Path(args.pkl).name.replace(".pkl", "_binaries_0.pkl")
        )
        path_dest_1 = Path(args.pkl).with_name(
            Path(args.pkl).name.replace(".pkl", "_binaries_1.pkl")
        )

        prev_to_check: List[Path] = []
        if path_dest_0.exists():
            prev_to_check.append(path_dest_0)

        if path_dest_1.exists():
            prev_to_check.append(path_dest_1)

        if len(prev_to_check) > 0:
            # sort the files based on their last edit time
            prev_to_check.sort(key=lambda x: x.stat().st_mtime)

        # try to load the files one by one until one is loaded starting from the most recent
        for path in reversed(prev_to_check):
            try:
                logging.info(f"Loading functions from {path}")
                with open(path, "rb") as f:
                    generated_functions = pickle.load(f)
                logging.info(f"Loaded {len(generated_functions)} functions from {path}")
                name = path.name.replace("_binaries_0.pkl", "").replace(
                    "_binaries_1.pkl", ""
                )
                path_dest = path.with_name(name)
                break
            except:
                logging.info(f"Could not load functions from {path}")
                pass

    else:
        path_dest = Path(args.dest)

    functions_names = list(cpps.keys())
    functions_names.sort()
    total = len(functions_names)
    logging.info(f"Total number of functions: {len(functions_names)}")

    already_generated = set(generated_functions.keys())
    logging.info(f"Already generated {len(already_generated)} functions")

    functions_names = [
        name for name in functions_names if name not in already_generated
    ]

    functions_names = functions_names

    functions_tuples = [(name, cpps[name]) for name in functions_names]

    logging.info(f"Generating {len(functions_names)} functions")

    nbr_saves = 0

    logging.info("Compiling HermesII library")

    # check that cmakefile of src contains HermesII
    cmake_src_path = Path("./src/CMakeLists.txt")
    with open(cmake_src_path, "r") as f:
        cmake_src_content = f.readlines()

    cmake_src_content = [
        line.replace("# ", "") if "HermesII" in line else line
        for line in cmake_src_content
    ]

    with open(cmake_src_path, "w") as f:
        f.writelines(cmake_src_content)

    # build HermesII and store it in the tmp folder
    script = """
    rm -fr build && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make HermesII -j && \
    cd ..
    cp build/src/libHermesII.a {libHermesII_path}
    """

    try:
        start_time = time.time()
        output = subprocess.check_output(
            script.format(libHermesII_path=args.libHermesII_path),
            shell=True,
            stderr=subprocess.STDOUT,
        )
        end_time = time.time()
        logging.info(
            f"Compilation time of HermesII in seconds: {end_time - start_time}"
        )
    except subprocess.CalledProcessError as e:
        output = e.output
        stderr = e.stderr
        stdout = e.stdout
        logging.error("Error compiling HermesII")
        logging.error(output)
        logging.error(stderr)
        logging.error(stdout)
        raise e

    # remove HermesII from the cmakefile of src
    cmake_src_content = [
        line if "HermesII" not in line else "# " + line for line in cmake_src_content
    ]

    with open(cmake_src_path, "w") as f:
        f.writelines(cmake_src_content)

    for i in tqdm(range(0, len(functions_tuples), args.batch_size)):
        logging.info(f"Generating functions from {i} to {i + args.batch_size}")
        clean_functions()
        generate_functions(
            functions_tuples[i : i + args.batch_size], args.libHermesII_path
        )
        logging.info("Compiling functions")
        start_time = time.time()
        # compile_functions()
        end_time = time.time()
        logging.info(f"Compilation time in seconds: {end_time - start_time}")
        logging.info("Storing generated functions")
        # store_generated_functions(generated_functions)
        logging.info(f"Generated {i + args.batch_size} functions")
        if (i / args.batch_size) % args.saving_frequency == 0:
            logging.info("Saving generated functions in a pickle file")
            # save in 2 files based on i being even or odd
            path_to_save = path_dest.with_name(
                path_dest.name + f"_binaries_{nbr_saves % 2}.pkl"
            )
            start_time = time.time()

            with open(path_to_save, "wb") as f:
                pickle.dump(generated_functions, f, protocol=pickle.HIGHEST_PROTOCOL)

            end_time = time.time()
            logging.info(f"Saving time in seconds: {end_time - start_time}")

            nbr_saves += 1
        logging.info("=========================")

    # store the generated functions
    with open(path_to_save, "wb") as f:
        pickle.dump(generated_functions, f, protocol=pickle.HIGHEST_PROTOCOL)
