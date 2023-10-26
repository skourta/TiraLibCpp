from google.protobuf.internal import enum_type_wrapper as _enum_type_wrapper
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from typing import ClassVar as _ClassVar, Optional as _Optional, Union as _Union

DESCRIPTOR: _descriptor.FileDescriptor

class Operation(int, metaclass=_enum_type_wrapper.EnumTypeWrapper):
    __slots__ = []
    LEGALITY: _ClassVar[Operation]
    EXECUTION: _ClassVar[Operation]
LEGALITY: Operation
EXECUTION: Operation

class TiramisuProgram(_message.Message):
    __slots__ = ["name", "schedule", "operation"]
    NAME_FIELD_NUMBER: _ClassVar[int]
    SCHEDULE_FIELD_NUMBER: _ClassVar[int]
    OPERATION_FIELD_NUMBER: _ClassVar[int]
    name: str
    schedule: str
    operation: Operation
    def __init__(self, name: _Optional[str] = ..., schedule: _Optional[str] = ..., operation: _Optional[_Union[Operation, str]] = ...) -> None: ...

class TiramisuResult(_message.Message):
    __slots__ = ["name", "legality", "isl_ast", "execution_times"]
    NAME_FIELD_NUMBER: _ClassVar[int]
    LEGALITY_FIELD_NUMBER: _ClassVar[int]
    ISL_AST_FIELD_NUMBER: _ClassVar[int]
    EXECUTION_TIMES_FIELD_NUMBER: _ClassVar[int]
    name: str
    legality: bool
    isl_ast: str
    execution_times: str
    def __init__(self, name: _Optional[str] = ..., legality: bool = ..., isl_ast: _Optional[str] = ..., execution_times: _Optional[str] = ...) -> None: ...
