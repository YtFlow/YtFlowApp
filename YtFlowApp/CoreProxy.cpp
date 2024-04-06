#include "pch.h"
#include "CoreProxy.h"

#include <optional>
#include <string>

#include "CoreFfi.h"

namespace winrt::YtFlowApp::implementation
{
    template <class... Ts> struct overloaded : Ts...
    {
        using Ts::operator()...;
    };

    using ytflow_core::ytflow_app_proxy_data_proxy_analyze;
    using ytflow_core::ytflow_app_proxy_data_proxy_compose_v1;
    using ytflow_core::ytflow_app_share_link_decode;
    using ytflow_core::ytflow_app_share_link_encode;

    struct FfiProxyNameOnly
    {
        std::string name;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiProxyNameOnly, name)

    void DecodeFfiProxyLegCbor(nlohmann::json const &json, FfiProxyLeg &r)
    {
        auto const protocolIt = std::begin(json["protocol"]);
        if (protocolIt.key() == "Shadowsocks")
        {
            r.protocol = protocolIt.value().get<FfiProxyProtocolShadowsocks>();
        }
        else if (protocolIt.key() == "Trojan")
        {
            r.protocol = protocolIt.value().get<FfiProxyProtocolTrojan>();
        }
        else if (protocolIt.key() == "Http")
        {
            r.protocol = protocolIt.value().get<FfiProxyProtocolHttp>();
        }
        else if (protocolIt.key() == "Socks5")
        {
            r.protocol = protocolIt.value().get<FfiProxyProtocolSocks5>();
        }
        else if (protocolIt.key() == "VMess")
        {
            r.protocol = protocolIt.value().get<FfiProxyProtocolVMess>();
        }

        json["dest"].get_to(r.dest);

        if (nlohmann::json const obfsDoc = json.value("obfs", nlohmann::json());

            obfsDoc != nullptr)
        {
            auto const obfsIt = std::ranges::begin(obfsDoc);
            if (obfsIt.key() == "HttpObfs")
            {
                r.obfs = obfsIt.value().get<FfiProxyObfsHttpObfs>();
            }
            else if (obfsIt.key() == "TlsObfs")
            {
                r.obfs = obfsIt.value().get<FfiProxyObfsTlsObfs>();
            }
            else if (obfsIt.key() == "WebSocket")
            {
                r.obfs = obfsIt.value().get<FfiProxyObfsWebSocket>();
            }
        }

        if (nlohmann::json const tlsDoc = json.value("tls", nlohmann::json());

            tlsDoc != nullptr)
        {
            r.tls = {tlsDoc.get<FfiProxyTls>()};
        }
    }
    void EncodeFfiProxyLegCbor(nlohmann::json &json, FfiProxyLeg const &r)
    {
        json["protocol"] = std::visit(overloaded{
                                          [](FfiProxyProtocolShadowsocks const &v) {
                                              return nlohmann::json({{"Shadowsocks", v}});
                                          },
                                          [](FfiProxyProtocolTrojan const &v) {
                                              return nlohmann::json({{"Trojan", v}});
                                          },
                                          [](FfiProxyProtocolHttp const &v) {
                                              return nlohmann::json({{"Http", v}});
                                          },
                                          [](FfiProxyProtocolSocks5 const &v) {
                                              return nlohmann::json({{"Socks5", v}});
                                          },
                                          [](FfiProxyProtocolVMess const &v) {
                                              return nlohmann::json({{"VMess", v}});
                                          },
                                      },
                                      r.protocol);
        json["dest"] = r.dest;
        if (r.obfs.has_value())
        {
            json["obfs"] = std::visit(overloaded{
                                          [](FfiProxyObfsHttpObfs const &v) {
                                              return nlohmann::json({{"HttpObfs", v}});
                                          },
                                          [](FfiProxyObfsTlsObfs const &v) {
                                              return nlohmann::json({{"TlsObfs", v}});
                                          },
                                          [](FfiProxyObfsWebSocket const &v) {
                                              return nlohmann::json({{"WebSocket", v}});
                                          },
                                      },
                                      r.obfs.value());
        }
        if (r.tls.has_value())
        {
            json["tls"] = r.tls.value();
        }
    }

    std::string ExtractProxyNameFromCbor(std::span<uint8_t const> cbor)
    {
        return nlohmann::json::from_cbor(cbor).get<FfiProxyNameOnly>().name;
    }
    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertShareLinkToDataProxyV1(std::string const &link)
    {
        try
        {
            auto const proxy_buffer = unwrap_ffi_byte_buffer(ytflow_app_share_link_decode(link.c_str()));
            auto name = ExtractProxyNameFromCbor(proxy_buffer);
            auto data_buffer = unwrap_ffi_byte_buffer(
                ytflow_app_proxy_data_proxy_compose_v1(proxy_buffer.data(), proxy_buffer.size()));
            return std::make_pair(std::move(name), std::move(data_buffer));
        }
        catch (FfiException const &)
        {
            return std::nullopt;
        }
    }

    std::optional<std::string> ConvertDataProxyToShareLink(std::string const &name, std::span<uint8_t const> data_proxy,
                                                           uint16_t version)
    {
        try
        {
            auto const proxy_buf = unwrap_ffi_byte_buffer(
                ytflow_app_proxy_data_proxy_analyze(name.c_str(), data_proxy.data(), data_proxy.size(), version));
            auto link = unwrap_ffi_string(ytflow_app_share_link_encode(proxy_buf.data(), proxy_buf.size()));
            return std::make_optional(std::move(link));
        }
        catch (FfiException const &)
        {
            return std::nullopt;
        }
    }
}
