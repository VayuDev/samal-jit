#include "samal_lib/ExternalVMValue.hpp"
#include "samal_lib/Pipeline.hpp"
#include <iostream>

int main(int argc, char** argv) {
    samal::Pipeline pipeline;
    pipeline.addFile("samal_code/lib/Core.samal");
    pipeline.addFile("samal_code/examples/Templ.samal");
    pipeline.addFile("samal_code/examples/Lists.samal");
    pipeline.addFile("samal_code/examples/Structs.samal");
    pipeline.addFile("samal_code/examples/Euler.samal");
    pipeline.addFile("samal_code/examples/Main.samal");
    pipeline.addNativeFunction(samal::NativeFunction{
        "Core.print",
        samal::Datatype::createFunctionType(samal::Datatype::createEmptyTuple(), { samal::Datatype::createUndeterminedIdentifierType("T") }),
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            std::cout << "[Code] " << params.at(0).dump() << "\n";
            return samal::ExternalVMValue::wrapEmptyTuple(vm);
        } });
    pipeline.addNativeFunction(samal::NativeFunction{
        "Core.print",
        samal::Datatype::createFunctionType(samal::Datatype::createEmptyTuple(), { samal::Datatype::createSimple(samal::DatatypeCategory::i32) }),
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            printf("[Code i32] %i\n", params.at(0).as<int32_t>());
            return samal::ExternalVMValue::wrapEmptyTuple(vm);
        } });
    pipeline.addNativeFunction(samal::NativeFunction{
        "Core.print",
        samal::Datatype::createFunctionType(samal::Datatype::createEmptyTuple(), { samal::Datatype::createSimple(samal::DatatypeCategory::i64) }),
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            printf("[Code i64] %li\n", params.at(0).as<int64_t>());
            return samal::ExternalVMValue::wrapEmptyTuple(vm);
        } });
    pipeline.addNativeFunction(samal::NativeFunction{
        "Core.dumpStackTrace",
        samal::Datatype::createFunctionType(samal::Datatype::createEmptyTuple(), {}),
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            std::cout << "[Stacktrace] " << vm.dumpVariablesOnStack();
            return samal::ExternalVMValue::wrapEmptyTuple(vm);
        } });
    pipeline.addNativeFunction(samal::NativeFunction{
        "Core.toI32",
        samal::Datatype::createFunctionType(samal::Datatype::createSimple(samal::DatatypeCategory::i32), { samal::Datatype::createSimple(samal::DatatypeCategory::char_) }),
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            return samal::ExternalVMValue::wrapInt32(vm,  params.at(0).as<int32_t>());
        } });
    pipeline.addNativeFunction(samal::NativeFunction{
        "Core.toChar",
        samal::Datatype::createFunctionType(samal::Datatype::createSimple(samal::DatatypeCategory::char_), { samal::Datatype::createSimple(samal::DatatypeCategory::i32) }),
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            return samal::ExternalVMValue::wrapChar(vm,  params.at(0).as<int32_t>());
        } });
    samal::VM vm = pipeline.compile();
    std::cout << vm.getProgram().disassemble() << "\n";
    samal::Stopwatch vmStopwatch{ "VM execution" };
    auto ret = vm.run("Main.main", std::vector<samal::ExternalVMValue>{ samal::ExternalVMValue::wrapInt32(vm, 5) });
    vmStopwatch.stop();
    std::cout << "Func2 returned " << ret.dump() << "\n";
}