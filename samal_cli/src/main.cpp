#include "samal_lib/ExternalVMValue.hpp"
#include "samal_lib/Pipeline.hpp"
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <filesystem>

int main(int argc, char** argv) {
    using namespace samal;
    Pipeline pl;
    pl.addFile("samal_code/lib/Core.samal");
    pl.addFile("samal_code/lib/IO.samal");
    pl.addFile("samal_code/examples/Templ.samal");
    pl.addFile("samal_code/examples/Lists.samal");
    pl.addFile("samal_code/examples/Structs.samal");
    pl.addFile("samal_code/examples/Euler.samal");
    pl.addFile("samal_code/examples/Main.samal");
    pl.addFile("samal_code/examples/Server.samal");
    pl.addNativeFunction(samal::NativeFunction{
        "Core.print",
        pl.incompleteType("fn(T) -> ()"),
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            std::cout << "[Code] " << params.at(0).dump() << "\n";
            return samal::ExternalVMValue::wrapEmptyTuple(vm);
        } });
    pl.addNativeFunction(samal::NativeFunction{
        "Core.print",
        pl.type("fn(i32) -> ()"),
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            printf("[Code i32] %i\n", params.at(0).as<int32_t>());
            return samal::ExternalVMValue::wrapEmptyTuple(vm);
        } });
    pl.addNativeFunction(samal::NativeFunction{
        "Core.print",
        pl.type("fn(i64) -> ()"),
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            printf("[Code i64] %li\n", params.at(0).as<int64_t>());
            return samal::ExternalVMValue::wrapEmptyTuple(vm);
        } });
    pl.addNativeFunction(samal::NativeFunction{
        "Core.dumpStackTrace",
        pl.type("fn() -> ()"),
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            std::cout << "[Stacktrace] " << vm.dumpVariablesOnStack();
            return samal::ExternalVMValue::wrapEmptyTuple(vm);
        } });
    pl.addNativeFunction(samal::NativeFunction{
        "Core.toI32",
        pl.type("fn(char) -> i32"),
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            return samal::ExternalVMValue::wrapInt32(vm,  params.at(0).as<int32_t>());
        } });
    pl.addNativeFunction(samal::NativeFunction{
        "Core.toChar",
        pl.type("fn(i32) -> char"),
        [](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            return samal::ExternalVMValue::wrapChar(vm,  params.at(0).as<int32_t>());
        } });
    pl.addNativeFunction(NativeFunction{
        "Core.toString",
        pl.type("fn(i32) -> [char]"),
        [](VM& vm, const std::vector<ExternalVMValue>& params) -> ExternalVMValue {
            std::string ret = std::to_string(params.at(0).as<int32_t>());
            return ExternalVMValue::wrapString(vm, ret);
        }});
    // io functions
    auto maybeStringType = pl.type("Core.Maybe<[char]>");
    pl.addNativeFunction(NativeFunction{
        "IO.readFileAsString",
        pl.type("fn([char]) -> Core.Maybe<[char]>"),
        [maybeStringType](VM& vm, const std::vector<ExternalVMValue>& params) -> ExternalVMValue {
            auto path = params.at(0).toCPPString();
            if(!std::filesystem::is_regular_file(path)) {
                return ExternalVMValue::wrapEnum(vm, maybeStringType, "None", {});
            }
            std::ifstream file(path);
            if(!file) {
                return ExternalVMValue::wrapEnum(vm, maybeStringType, "None", {});
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            return ExternalVMValue::wrapEnum(vm, maybeStringType, "Some", {ExternalVMValue::wrapString(vm, buffer.str())});
        }});

    auto maybeI32Type = pl.type("Core.Maybe<i32>");
    // Server functions
    pl.addNativeFunction(NativeFunction{
        "Server.openServerSocket",
        pl.type("fn(i32) -> Core.Maybe<i32>"),
        [maybeI32Type](VM& vm, const std::vector<ExternalVMValue>& params) -> ExternalVMValue {
            int32_t sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == -1) {
                printf("Failed to open socket\n");
                return ExternalVMValue::wrapEnum(vm, maybeI32Type, "None", {});
            }
            sockaddr_in serverAddress;
            memset(&serverAddress, 0, sizeof(serverAddress));

            serverAddress.sin_family = AF_INET;
            serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
            serverAddress.sin_port = htons(params.at(0).as<int32_t>());

            if ((bind(sock, (const sockaddr*) &serverAddress, sizeof(serverAddress))) != 0) {
                printf("Failed to open socket\n");
                return ExternalVMValue::wrapEnum(vm, maybeI32Type, "None", {});
            }

            if ((listen(sock, 3)) != 0) {
                printf("Failed to open socket\n");
                return ExternalVMValue::wrapEnum(vm, maybeI32Type, "None", {});
            }
            return ExternalVMValue::wrapEnum(vm, maybeI32Type, "Some", {ExternalVMValue::wrapInt32(vm, sock)});
        }});
    pl.addNativeFunction(NativeFunction{
        "Server.acceptClientSocket",
        pl.type("fn(i32) -> i32"),
        [](VM& vm, const std::vector<ExternalVMValue>& params) -> ExternalVMValue {
            auto sock = params.at(0).as<int32_t>();
            if(sock == 0) {
                return ExternalVMValue::wrapInt32(vm, 0);
            }
            sockaddr_in clientAddress{};
            socklen_t clientAddressLen = sizeof(clientAddress);
            int32_t conn = accept(params.at(0).as<int32_t>(), (sockaddr*)&clientAddress, &clientAddressLen);
            if (conn < 0) {
                return ExternalVMValue::wrapInt32(vm, 0);
            }
            timeval timeout{};
            timeout.tv_sec = 20;
            timeout.tv_usec = 0;

            if (setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
                return ExternalVMValue::wrapInt32(vm, 0);

            if (setsockopt(conn, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
                return ExternalVMValue::wrapInt32(vm, 0);

            return ExternalVMValue::wrapInt32(vm, conn);
        }});
    pl.addNativeFunction(NativeFunction{
        "Server.readStringUntilEmptyHeader",
        pl.type("fn(i32) -> [char]"),
        [](VM& vm, const std::vector<ExternalVMValue>& params) -> ExternalVMValue {
            auto sock = params.at(0).as<int32_t>();
            // TODO read whole utf-8 codepoint
            std::string ret;
            while(true) {
                char buf = -1;
                if(read(sock, &buf, 1) <= 0) {
                    return ExternalVMValue::wrapString(vm, ret);
                }
                ret += buf;
                if(ret.size() >= 4 && std::string_view{ret}.substr(ret.size() - 4) == "\r\n\r\n") {
                    return ExternalVMValue::wrapString(vm, ret);
                }
            }
        }});
    pl.addNativeFunction(NativeFunction{
        "Server.readChar",
        pl.type("fn(i32) -> char"),
        [](VM& vm, const std::vector<ExternalVMValue>& params) -> ExternalVMValue {
            auto sock = params.at(0).as<int32_t>();
            char buf = -1;
            // TODO read whole utf-8 codepoint
            if(read(sock, &buf, 1) <= 0) {
                return ExternalVMValue::wrapChar(vm, 0);
            }
            return ExternalVMValue::wrapChar(vm, buf);
        }});
    pl.addNativeFunction(NativeFunction{
        "Server.writeString",
        pl.type("fn(i32, [char]) -> ()"),
        [](VM& vm, const std::vector<ExternalVMValue>& params) -> ExternalVMValue {
            auto sock = params.at(0).as<int32_t>();
            auto string = params.at(1).toCPPString();
            write(sock, string.c_str(), string.size());
            return ExternalVMValue::wrapEmptyTuple(vm);
        }});
    pl.addNativeFunction(NativeFunction{
        "Server.closeSocket",
        pl.type("fn(i32) -> ()"),
        [](VM& vm, const std::vector<ExternalVMValue>& params) -> ExternalVMValue {
            auto sock = params.at(0).as<int32_t>();
            close(sock);
            return ExternalVMValue::wrapEmptyTuple(vm);
        }});


    samal::VM vm = pl.compile();
    std::cout << vm.getProgram().disassemble() << "\n";
    samal::Stopwatch vmStopwatch{ "VM execution" };
    auto ret = vm.run("Server.main", std::vector<samal::ExternalVMValue>{ });
    vmStopwatch.stop();
    std::cout << "Func2 returned " << ret.dump() << "\n";
}