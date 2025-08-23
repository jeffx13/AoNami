#pragma once

#include "base/network/network.h"
#include "wasmtime/wasmtime.hh"
#include "app/appexception.h"
#include <QFile>
#include <QUrl>

class WasmEngine
{
public:
    static WasmEngine fromLocalFile(const QUrl &path) {
        if (!path.isLocalFile() || path.isEmpty()) {
            throw AppException("Expecting local file path.", "WasmEngine");
        }
        QString filePath = path.toLocalFile();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            throw AppException("Could not open file: " + filePath, "WasmEngine");
        }
        QByteArray fileData = file.readAll();
        if (fileData.isEmpty()) {
            throw AppException("File is empty", "WasmEngine");
        }
        auto buffer = std::vector<uint8_t>(fileData.begin(), fileData.end());
        return WasmEngine(buffer);
    }

    static WasmEngine fromOnlineFile(Client *client, const QString &url) {
        auto response = client->get(url, {}, {}, true);
        if (response.code != 200) {
            throw AppException("Failed to fetch WebAssembly file.", "WasmEngine");
        }
        return WasmEngine(response.content);
    }


    WasmEngine(std::vector<uint8_t> &wasmBuffer) {
        module = wasmtime::Module::compile(engine, wasmBuffer).ok();
        if (!module) {
            throw AppException("Failed to compile WebAssembly module.", "WasmEngine");
        }

        wasmtime::Func f(
            store, wasmtime::FuncType({wasmtime::ValKind::I32, wasmtime::ValKind::I32, wasmtime::ValKind::I32, wasmtime::ValKind::I32}, {}),
            [](auto caller, auto params, auto results) -> auto{
                return std::monostate();
            });
        instance = wasmtime::Instance::create(store, module.value(), {f}).unwrap();
    }
    wasmtime::Engine engine;
    wasmtime::Store store {engine};
    std::optional<wasmtime::Module> module;
    std::optional<wasmtime::Instance> instance;

private:
    // remove copy etc.
    // WasmEngine(const WasmEngine&) = delete;
    // WasmEngine& operator=(const WasmEngine&) = delete;
    // WasmEngine(WasmEngine&&) = delete;
    // WasmEngine& operator=(WasmEngine&&) = delete;

};
