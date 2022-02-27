#include "pch.h"
#include "CoreRpc.h"

#include <cbor.h>

#include "VectorBuffer.h"

using namespace nlohmann;
using namespace winrt;
using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

namespace winrt::YtFlowApp::implementation
{
    const char *RpcException::what() const throw()
    {
        return msg.data();
    }

    bool RpcPluginInfo::operator==(RpcPluginInfo const &that) const
    {
        return id == that.id && hashcode == that.hashcode;
    }

    CoreRpc CoreRpc::Connect()
    {
        StreamSocket socket;
        socket.Control().NoDelay(true);
        socket.ConnectAsync(HostName{L"127.0.0.1"}, L"9097").get();
        return CoreRpc(std::move(socket));
    }

    std::vector<uint8_t> CoreRpc::ReadChunk(IInputStream stream)
    {
        Windows::Storage::Streams::DataReader reader{std::move(stream)};
        reader.ByteOrder(ByteOrder::BigEndian);
        uint32_t readLen{4};
        while (readLen > 0)
        {
            auto const len{reader.LoadAsync(readLen).get()};
            if (len == 0)
            {
                RpcException ex;
                ex.msg = "RPC EOF";
                throw ex;
            }
            readLen -= len;
        }
        uint32_t const chunkSize{reader.ReadUInt32()};
        while (readLen < chunkSize)
        {
            auto const len{reader.LoadAsync(chunkSize - readLen).get()};
            if (len == 0)
            {
                RpcException ex;
                ex.msg = "RPC EOF";
                throw ex;
            }
            readLen += len;
        }
        std::vector<uint8_t> ret(chunkSize);
        reader.ReadBytes(ret);
        reader.DetachStream();
        reader.Close();
        return ret;
    }

    std::vector<RpcPluginInfo> CoreRpc::CollectAllPluginInfo(std::map<uint32_t, uint32_t> const &hashcodes) const &
    {
        auto const writeStream{m_socket.OutputStream()};
        auto readStream{m_socket.InputStream()};
        auto const ioLock{m_ioLock};

        std::lock_guard _scope{*ioLock};

        // Use tinycbor instead of nlohmann/json here because the latter does
        // not support using a non-string value as key.
        CborEncoder enc, mainMapEnc, cMapEnc, hashMapEnc;
        std::vector<uint8_t> reqData(hashcodes.size() * 10 + 16);
        cbor_encoder_init(&enc, reqData.data() + 4, reqData.size() - 4, 0);
        assert(cbor_encoder_create_map(&enc, &mainMapEnc, 1) == CborNoError);
        {
            assert(cbor_encode_text_string(&mainMapEnc, "c", 1) == CborNoError);
            assert(cbor_encoder_create_map(&mainMapEnc, &cMapEnc, 1) == CborNoError);
            {
                assert(cbor_encode_text_string(&cMapEnc, "h", 1) == CborNoError);
                assert(cbor_encoder_create_map(&cMapEnc, &hashMapEnc, hashcodes.size()) == CborNoError);
                for (auto const &[k, v] : hashcodes)
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
        writeStream.WriteAsync(std::move(reqBuf)).get();
        writeStream.FlushAsync().get();

        auto const resData{ReadChunk(std::move(readStream))};
        auto const res{json::from_cbor(resData)};
        if (res["c"] != "Ok")
        {
            RpcException ex;
            ex.msg = res["e"];
            throw ex;
        }

        std::vector<RpcPluginInfo> const ret = std::move(res["d"]);
        return ret;
    }

}
