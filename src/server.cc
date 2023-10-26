#include <bits/stdc++.h>
#include <grpcpp/grpcpp.h>

#include <string>

#include "tiramisuserver.grpc.pb.h"

#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>

#include <hermes_ii/utils.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using tiramisuserver::TiramisuProgram;
using tiramisuserver::TiramisuResult;
using tiramisuserver::TiramisuServer;

using namespace tiramisu;

bool function_gramschmidt()
{

    tiramisu::init("function_gramschmidt_LARGE");

    // -------------------------------------------------------
    // Layer I
    // -------------------------------------------------------

    // Iteration variables
    var i("i", 0, 1000), j("j", 0, 1200), k("k", 0, 1200), l("l"), m("m");

    // inputs
    input A("A", {i, k}, p_float64);
    input Q("Q", {i, k}, p_float64);
    input R("R", {j, j}, p_float64);
    input nrm("nrm", {}, p_float64);

    // Computations
    computation nrm_init("{nrm_init[k]: 0<=k<1200}", expr(), true, p_float64, global::get_implicit_function());
    nrm_init.set_expression(0.000001);

    computation nrm_comp("{nrm_comp[k,i]: 0<=k<1200 and 0<=i<1000}", expr(), true, p_float64, global::get_implicit_function());
    nrm_comp.set_expression(nrm(0) + A(i, k) * A(i, k));

    computation R_diag("{R_diag[k]: 0<=k<1200}", expr(), true, p_float64, global::get_implicit_function());
    R_diag.set_expression(expr(o_sqrt, nrm(0)));

    computation Q_out("{Q_out[k,i]: 0<=k<1200 and 0<=i<1000}", expr(), true, p_float64, global::get_implicit_function());
    Q_out.set_expression(A(i, k) / R(k, k));

    computation R_up_init("{R_up_init[k,j]: 0<=k<1200 and k+1<=j<1200}", expr(), true, p_float64, global::get_implicit_function());
    R_up_init.set_expression(0.0);

    computation R_up("{R_up[k,j,i]: 0<=k<1200 and k+1<=j<1200 and 0<=i<1000}", expr(), true, p_float64, global::get_implicit_function());
    R_up.set_expression(R(k, j) + Q(i, k) * A(i, j));

    computation A_out("{A_out[k,j,i]: 0<=k<1200 and k+1<=j<1200 and 0<=i<1000}", expr(), true, p_float64, global::get_implicit_function());
    A_out.set_expression(A(i, j) - Q(i, k) * R(k, j));

    // -------------------------------------------------------
    // Layer II
    // -------------------------------------------------------
    nrm_init.then(nrm_comp, k)
        .then(R_diag, k)
        .then(Q_out, k)
        .then(R_up_init, k)
        .then(R_up, j)
        .then(A_out, j);

    // -------------------------------------------------------
    // Layer III
    // -------------------------------------------------------
    // Input Buffers
    buffer b_A("b_A", {1000, 1200}, p_float64, a_output);
    buffer b_nrm("b_nrm", {1}, p_float64, a_temporary);
    buffer b_R("b_R", {1200, 1200}, p_float64, a_output);
    buffer b_Q("b_Q", {1000, 1200}, p_float64, a_output);

    // Store inputs
    A.store_in(&b_A);
    Q.store_in(&b_Q);
    R.store_in(&b_R);
    nrm.store_in(&b_nrm);

    // Store computations
    nrm_init.store_in(&b_nrm, {});
    nrm_comp.store_in(&b_nrm, {});
    R_diag.store_in(&b_R, {k, k});
    Q_out.store_in(&b_Q, {i, k});
    R_up_init.store_in(&b_R);
    R_up.store_in(&b_R, {k, j});
    A_out.store_in(&b_A, {i, j});

    // -------------------------------------------------------
    // Code Generation
    // -------------------------------------------------------
    prepare_schedules_for_legality_checks();
    perform_full_dependency_analysis();
    bool is_legal = true;

    A_out.loop_reversal(2);
    is_legal &= loop_parallelization_is_legal(1, {&R_up_init, &R_up, &A_out});
    R_up_init.tag_parallel_level(1);
    R_up_init.tile(1, 128);
    R_up.tile(1, 2, 128, 128);
    A_out.tile(1, 2, 128, 128);

    prepare_schedules_for_legality_checks();

    is_legal &= check_legality_of_function();
    return is_legal;
}

// Server Implementation
class TiramisuServerImpl final : public TiramisuServer::Service
{
    Status getLegality(ServerContext *context, const TiramisuProgram *request,
                       TiramisuResult *reply) override
    {
        // Obtains the original string from the request
        std::string function_name = request->name();
        Operation operation = request->operation() == tiramisuserver::Operation::LEGALITY ? Operation::legality : Operation::execution;
        //// FuncMap
        // auto funcMap = createFuncMap();
        // auto res = funcMap[function_name](request->schedule(), operation);

        //// Switch
        // auto res = switchFunction(function_name, request->schedule(), operation);

        //// Double Switch
        auto switchMap = createSwitchMap();
        std::string function_number = function_name.substr(8);
        int function_number_int = std::stoi(function_number);
        int key = function_number_int % 300;
        auto res = switchMap[key](function_name, request->schedule(), operation);
        if (!res.success)
        {
            std::cout << "Error: " << res.name << std::endl;
            return Status::CANCELLED;
        }

        // Sets the reply
        reply->set_name(res.name);
        reply->set_legality(res.legality);
        reply->set_execution_times(res.exec_times);
        return Status::OK;
    }
};

void RunServer()
{
    std::string server_address("0.0.0.0:50051");
    TiramisuServerImpl service;

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which
    // communication with client takes place
    builder.RegisterService(&service);

    // Assembling the server
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on port: " << server_address << std::endl;

    server->Wait();
}

int main(int argc, char **argv)
{
    RunServer();
    return 0;
}