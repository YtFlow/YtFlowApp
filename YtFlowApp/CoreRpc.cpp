#include "pch.h"
#include "CoreRpc.h"

#include <tinycbor/cbor.h>

#include "VectorBuffer.h"

using namespace nlohmann;
using namespace winrt;
using namespace Windows::Networking;
using namespace Sockets;
using namespace Windows::Storage::Streams;

namespace winrt::YtFlowApp::implementation
{
    using concurrency::task;

    const char *RpcException::what() const throw()
    {
        return msg.data();
    }

    bool RpcPluginInfo::operator==(RpcPluginInfo const &that) const
    {
        return id == that.id && hashcode == that.hashcode;
    }

    task<CoreRpc> CoreRpc::Connect()
    {
        StreamSocket socket;
        socket.Control().NoDelay(true);
        auto const port = Windows::Storage::ApplicationData::Current()
                              .LocalSettings()
                              .Values()
                              .TryLookup(L"YTFLOW_CORE_RPC_PORT")
                              .try_as<hstring>()
                              .value_or(L"9097");
        co_await socket.ConnectAsync(HostName{L"127.0.0.1"}, port);
        co_return CoreRpc(std::move(socket));
    }

    task<std::vector<uint8_t>> CoreRpc::ReadChunk(IInputStream stream)
    {
        DataReader const reader{std::move(stream)};
        reader.ByteOrder(ByteOrder::BigEndian);
        uint32_t readLen{4};
        while (readLen > 0)
        {
            auto const len = co_await reader.LoadAsync(readLen);
            if (len == 0)
            {
                RpcException ex;
                ex.msg = "RPC EOF";
                throw std::move(ex);
            }
            readLen -= len;
        }
        uint32_t const chunkSize{reader.ReadUInt32()};
        while (readLen < chunkSize)
        {
            auto const len = co_await reader.LoadAsync(chunkSize - readLen);
            if (len == 0)
            {
                RpcException ex;
                ex.msg = "RPC EOF";
                throw std::move(ex);
            }
            readLen += len;
        }
        std::vector<uint8_t> ret(chunkSize);
        reader.ReadBytes(ret);
        reader.DetachStream();
        reader.Close();
        co_return ret;
    }

    task<std::vector<RpcPluginInfo>> CoreRpc::CollectAllPluginInfo(
        std::shared_ptr<std::map<uint32_t, uint32_t>> hashcodes) const &
    {
        auto const writeStream{m_socket.OutputStream()};
        auto readStream{m_socket.InputStream()};
        auto const ioLock{m_ioLock};

        auto const _scope = co_await ioLock->scoped_lock_async();

        // Use tinycbor instead of nlohmann/json here because the latter does
        // not support using a non-string value as key.
        CborEncoder enc, mainMapEnc, cMapEnc, hashMapEnc;
        std::vector<uint8_t> reqData(hashcodes->size() * 10 + 16);
        cbor_encoder_init(&enc, reqData.data() + 4, reqData.size() - 4, 0);
        assert(cbor_encoder_create_map(&enc, &mainMapEnc, 1) == CborNoError);
        {
            assert(cbor_encode_text_string(&mainMapEnc, "c", 1) == CborNoError);
            assert(cbor_encoder_create_map(&mainMapEnc, &cMapEnc, 1) == CborNoError);
            {
                assert(cbor_encode_text_string(&cMapEnc, "h", 1) == CborNoError);
                assert(cbor_encoder_create_map(&cMapEnc, &hashMapEnc, hashcodes->size()) == CborNoError);
                for (auto const &[k, v] : *hashcodes)
                {
                    assert(cbor_encode_int(&hashMapEnc, k) == CborNoError);
                    assert(cbor_encode_int(&hashMapEnc, v) == CborNoError);
                }
                assert(cbor_encoder_close_container(&cMapEnc, &hashMapEnc) == CborNoError);
            }
            assert(cbor_encoder_close_container(&mainMapEnc, &cMapEnc) == CborNoError);
        }
        assert(cbor_encoder_close_container(&enc, &mainMapEnc) == CborNoError);
        auto const reqDataLen{cbor_encoder_get_buffer_size(&enc, reqData.data() + 4)};

        reqData[0] = static_cast<uint8_t>(reqDataLen >> 24);
        reqData[1] = static_cast<uint8_t>(reqDataLen >> 16);
        reqData[2] = static_cast<uint8_t>(reqDataLen >> 8);
        reqData[3] = static_cast<uint8_t>(reqDataLen);
        auto reqBuf{winrt::make<VectorBuffer>(std::move(reqData))};
        get_self<VectorBuffer>(reqBuf)->m_length = static_cast<uint32_t>(reqDataLen + 4);
        co_await writeStream.WriteAsync(std::move(reqBuf));
        co_await writeStream.FlushAsync();

        auto const resData = co_await ReadChunk(std::move(readStream));
        auto res{json::from_cbor(resData)};
        if (res.at("c") != "Ok")
        {
            RpcException ex;
            ex.msg = res.at("e");
            throw std::move(ex);
        }

        std::vector<RpcPluginInfo> const ret = std::move(res.at("d"));
        co_return ret;
    }

    task<std::vector<uint8_t>> CoreRpc::SendRequestToPlugin(uint32_t pluginIdParam, std::string_view funcParam,
                                                            std::vector<uint8_t> paramsParam) const &
    {
        auto const pluginId = pluginIdParam;
        auto const func = funcParam;
        auto const params = std::move(paramsParam);

        auto const writeStream{m_socket.OutputStream()};
        auto readStream{m_socket.InputStream()};
        auto const ioLock{m_ioLock};

        auto const _scope = co_await ioLock->scoped_lock_async();

        json reqDoc{"{ \"p\": {} }"_json};
        auto &reqDocInner{reqDoc["p"]};
        reqDocInner["id"] = pluginId;
        reqDocInner["fn"] = func;
        reqDocInner["p"] = json::binary_t{std::move(params)};
        auto reqData{json::to_cbor(reqDoc)};

        auto const reqDataLen{reqData.size()};
        reqData.insert(reqData.begin(), {static_cast<uint8_t>(reqDataLen >> 24), static_cast<uint8_t>(reqDataLen >> 16),
                                         static_cast<uint8_t>(reqDataLen >> 8), static_cast<uint8_t>(reqDataLen)});
        auto reqBuf{winrt::make<VectorBuffer>(std::move(reqData))};
        get_self<VectorBuffer>(reqBuf)->m_length = static_cast<uint32_t>(reqDataLen + 4);
        co_await writeStream.WriteAsync(std::move(reqBuf));
        co_await writeStream.FlushAsync();

        auto const resData = co_await ReadChunk(std::move(readStream));
        auto res{json::from_cbor(resData)};
        if (res.at("c") != "Ok")
        {
            RpcException ex;
            ex.msg = res.at("e");
            throw std::move(ex);
        }

        co_return std::vector(std::move(res.at("d")).get_binary());
    }

}
