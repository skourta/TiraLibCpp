import sys
import time

import grpc
from tiramisuserver_pb2 import Operation, TiramisuProgram
from tiramisuserver_pb2_grpc import TiramisuServerStub

# read function name from command line
function_name = sys.argv[1]
with grpc.insecure_channel("localhost:50051") as channel:
    stub = TiramisuServerStub(channel)
    # time the function call
    start = time.time()
    response = stub.getLegality(
        TiramisuProgram(
            name=function_name, schedule="", operation=Operation.LEGALITY
        )  # You can also specify a function name like function550013
    )
    end = time.time()

    print(
        f"Response received: \n name:{response.name}, \n legality:{response.legality}, \n execution_times:{response.execution_times} \n\n Time taken: {end-start}"
    )
