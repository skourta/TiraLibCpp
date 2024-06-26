#pragma once

#include <tiramisu/tiramisu.h>
#include <TiraLibCPP/utils.h>

bool apply_action(std::string action_str, tiramisu::function *implicit_function, Result &result);

bool apply_actions_from_schedule_str(std::string schedule_str, tiramisu::function *implicit_function, Result &result);

Result schedule_str_to_result(std::string function_name, std::string schedule_str, Operation operation, std::vector<tiramisu::buffer *> buffers);

void schedule_str_to_result_str(std::string function_name, std::string schedule_str, Operation operation, std::vector<tiramisu::buffer *> buffers);