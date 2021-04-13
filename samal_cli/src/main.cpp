#include "samal_lib/ExternalVMValue.hpp"
#include "samal_lib/Pipeline.hpp"
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <filesystem>
#include <random>

#include <csignal>
#ifdef SAMAL_ENABLE_GFX_CAIRO
#include <cairomm-1.0/cairomm/cairomm.h>
#endif

int main(int argc, char** argv) {
    using namespace samal;
    Pipeline pl;
    pl.addFile("samal_code/lib/Core.samal");
    pl.addFile("samal_code/lib/IO.samal");
    pl.addFile("samal_code/lib/Net.samal");
    pl.addFile("samal_code/lib/Gfx.samal");
    pl.addFile("samal_code/lib/Math.samal");
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
    auto maybeI32Type = pl.type("Core.Maybe<i32>");
    pl.addNativeFunction(samal::NativeFunction{
        "Core.toI32",
        pl.type("fn(char) -> Core.Maybe<i32>"),
        [&](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            return ExternalVMValue::wrapEnum(vm, maybeI32Type, "Some", {samal::ExternalVMValue::wrapInt32(vm,  params.at(0).as<int32_t>())});
        } });
    pl.addNativeFunction(samal::NativeFunction{
        "Core.toI32",
        pl.type("fn([char]) -> Core.Maybe<i32>"),
        [&](samal::VM& vm, const std::vector<samal::ExternalVMValue>& params) -> samal::ExternalVMValue {
            try {
                return ExternalVMValue::wrapEnum(vm, maybeI32Type, "Some", {ExternalVMValue::wrapInt32(vm, std::stoi(params.at(0).toCPPString()))});
            } catch(...) {
                return ExternalVMValue::wrapEnum(vm, maybeI32Type, "None", {});
            }
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

    // Server functions
    pl.addNativeFunction(NativeFunction{
        "Net.openServerSocket",
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
        "Net.acceptClientSocket",
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
        "Net.recvStringUntilEmptyHeader",
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
        "Net.recvChar",
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
        "Core.toBytes",
        pl.type("fn([char]) -> [byte]"),
        [](VM& vm, const std::vector<ExternalVMValue>& params) -> ExternalVMValue {
            return ExternalVMValue::wrapStringAsByteArray(vm, params.at(0).toCPPString());
        }});
    pl.addNativeFunction(NativeFunction{
        "Net.sendString",
        pl.type("fn(i32, [char]) -> ()"),
        [](VM& vm, const std::vector<ExternalVMValue>& params) -> ExternalVMValue {
            auto sock = params.at(0).as<int32_t>();
            auto string = params.at(1).toCPPString();
            write(sock, string.c_str(), string.size());
            return ExternalVMValue::wrapEmptyTuple(vm);
        }});
    pl.addNativeFunction(NativeFunction{
        "Net.sendBytes",
        pl.type("fn(i32, [byte]) -> ()"),
        [](VM& vm, const std::vector<ExternalVMValue>& params) -> ExternalVMValue {
            auto sock = params.at(0).as<int32_t>();
            auto string = params.at(1).toByteBuffer();
            write(sock, string.data(), string.size());
            return ExternalVMValue::wrapEmptyTuple(vm);
        }});
    pl.addNativeFunction(NativeFunction{
        "Net.closeSocket",
        pl.type("fn(i32) -> ()"),
        [](VM& vm, const std::vector<ExternalVMValue>& params) -> ExternalVMValue {
            auto sock = params.at(0).as<int32_t>();
            close(sock);
            return ExternalVMValue::wrapEmptyTuple(vm);
        }});
    pl.addNativeFunction(NativeFunction{
        "Math.randomInt",
        pl.type("fn(i32, i32) -> i32"),
        [](VM& vm, const std::vector<ExternalVMValue>& params) -> ExternalVMValue {
            std::random_device r;
            std::default_random_engine e1(r());
            std::uniform_int_distribution<int32_t> uniform_dist(params.at(0).as<int32_t>(), params.at(1).as<int32_t>());
            return ExternalVMValue::wrapInt32(vm, uniform_dist(e1));
        }});
    pl.addNativeFunction(NativeFunction{
        "Math.sqrt",
        pl.type("fn(i32) -> i32"),
        [](VM& vm, const std::vector<ExternalVMValue>& params) -> ExternalVMValue {
            auto result = static_cast<int32_t>(sqrt(params.at(0).as<int32_t>()));
            return ExternalVMValue::wrapInt32(vm, result);
        }});
    signal(SIGPIPE, [](int) {});
#ifdef SAMAL_ENABLE_GFX_CAIRO
    pl.addNativeFunction(NativeFunction{
        "Gfx.render",
        pl.type("fn(Gfx.Image) -> [byte]"),
        [](VM& vm, const std::vector<ExternalVMValue>& params) -> ExternalVMValue {
            auto& img = params.at(0).asStructValue();
            const auto w = img.findValue("width").as<int32_t>();
            const auto h = img.findValue("height").as<int32_t>();
            auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_RGB24, w, h);

            auto c = Cairo::Context::create(surface);
            auto commands = img.findValue("commands").toVector();
            for(auto it = commands.rbegin(); it != commands.rend(); ++it) {
                auto& enumValue = it->asEnumValue();
                auto& commandType = it->getEnumValueSelectedFieldName();
                if(commandType == "SetColor") {
                    c->set_source_rgb(enumValue.elements.at(0).as<uint8_t>() / 255.0,
                                      enumValue.elements.at(1).as<uint8_t>() / 255.0,
                                      enumValue.elements.at(2).as<uint8_t>() / 255.0);
                } else if(commandType == "DrawLine") {
                    c->move_to(enumValue.elements.at(0).as<int32_t>(), enumValue.elements.at(1).as<int32_t>());
                    c->line_to(enumValue.elements.at(2).as<int32_t>(), enumValue.elements.at(3).as<int32_t>());
                } else if(commandType == "SetLineWidth") {
                    c->set_line_width(enumValue.elements.at(0).as<int32_t>() / 10.0);
                } else if(commandType == "Stroke") {
                    c->stroke();
                } else {
                    assert(false);
                }
            }

            std::vector<uint8_t> outputBuffer;
            surface->write_to_png_stream([&](const unsigned char* data, unsigned int length) -> Cairo::ErrorStatus {
                outputBuffer.resize(outputBuffer.size() + length);
                memcpy(outputBuffer.data() + outputBuffer.size() - length, data, length);
                return Cairo::ErrorStatus::CAIRO_STATUS_SUCCESS;
            });
            return ExternalVMValue::wrapByteArray(vm, outputBuffer.data(), outputBuffer.size());
        }});
#endif

    samal::VM vm = pl.compile();
    std::cout << vm.getProgram().disassemble() << "\n";
    samal::Stopwatch vmStopwatch{ "VM execution" };
    auto ret = vm.run("Server.main", std::vector<samal::ExternalVMValue>{ });
    vmStopwatch.stop();
    std::cout << "Func2 returned " << ret.dump() << "\n";
}