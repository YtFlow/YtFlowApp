#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace winrt::YtFlowApp::implementation
{
    struct FfiProxyProtocolShadowsocks
    {
        std::string cipher;
        nlohmann::json::binary_t password;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiProxyProtocolShadowsocks, cipher, password);
    struct FfiProxyProtocolTrojan
    {
        nlohmann::json::binary_t password;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiProxyProtocolTrojan, password);
    struct FfiProxyProtocolHttp
    {
        nlohmann::json::binary_t username;
        nlohmann::json::binary_t password;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiProxyProtocolHttp, username, password);
    struct FfiProxyProtocolSocks5
    {
        nlohmann::json::binary_t username;
        nlohmann::json::binary_t password;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiProxyProtocolSocks5, username, password);
    struct FfiProxyProtocolVMess
    {
        nlohmann::json::binary_t user_id;
        uint16_t alter_id{};
        std::string security;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiProxyProtocolVMess, user_id, alter_id, security);

    struct FfiProxyDest
    {
        std::string host;
        uint16_t port = 1;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiProxyDest, host, port);

    struct FfiProxyObfsHttpObfs
    {
        std::string host;
        std::string path;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiProxyObfsHttpObfs, host, path);
    struct FfiProxyObfsTlsObfs
    {
        std::string host;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiProxyObfsTlsObfs, host);
    struct FfiProxyObfsWebSocket
    {
        std::optional<std::string> host;
        std::string path;
        std::map<std::string, std::string> headers;
    };
    inline void from_json(nlohmann::json const &json, FfiProxyObfsWebSocket &r)
    {
        if (nlohmann::json const hostDoc = json.value("host", nlohmann::json());

            hostDoc != nullptr)
        {
            r.host = {hostDoc.get<std::string>()};
        }

        json.at("path").get_to(r.path);
        json.at("headers").get_to(r.headers);
    }
    inline void to_json(nlohmann::json &json, FfiProxyObfsWebSocket const &r)
    {
        if (r.host.has_value())
        {
            json["host"] = r.host.value();
        }

        json["path"] = r.path;
        json["headers"] = r.headers;
    }

    struct FfiProxyTls
    {
        std::vector<std::string> alpn;
        std::optional<std::string> sni;
        std::optional<bool> skip_cert_check;
    };
    inline void from_json(nlohmann::json const &json, FfiProxyTls &r)
    {
        r.alpn = json.at("alpn").get<std::vector<std::string>>();
        if (nlohmann::json const sniDoc = json.value("sni", nlohmann::json());

            sniDoc != nullptr)
        {
            r.sni = std::make_optional(sniDoc.get<std::string>());
        }
        if (nlohmann::json const skipCertCheckDoc = json.value("skip_cert_check", nlohmann::json());

            skipCertCheckDoc != nullptr)
        {
            r.skip_cert_check = {skipCertCheckDoc.get<bool>()};
        }
    }
    inline void to_json(nlohmann::json &json, FfiProxyTls const &r)
    {
        json["alpn"] = r.alpn;
        if (r.sni.has_value())
        {
            json["sni"] = r.sni.value();
        }
        if (r.skip_cert_check.has_value())
        {
            json["skip_cert_check"] = r.skip_cert_check.value();
        }
    }

    struct FfiProxyLeg
    {
        std::variant<FfiProxyProtocolShadowsocks, FfiProxyProtocolTrojan, FfiProxyProtocolHttp, FfiProxyProtocolSocks5,
                     FfiProxyProtocolVMess>
            protocol;
        FfiProxyDest dest;
        std::optional<std::variant<FfiProxyObfsHttpObfs, FfiProxyObfsTlsObfs, FfiProxyObfsWebSocket>> obfs;
        std::optional<FfiProxyTls> tls;
    };
    void DecodeFfiProxyLegCbor(nlohmann::json const &json, FfiProxyLeg &r);
    void EncodeFfiProxyLegCbor(nlohmann::json &json, FfiProxyLeg const &r);
    inline void from_json(nlohmann::json const &json, FfiProxyLeg &r)
    {
        DecodeFfiProxyLegCbor(json, r);
    }
    inline void to_json(nlohmann::json &json, FfiProxyLeg const &r)
    {
        EncodeFfiProxyLegCbor(json, r);
    }

    struct FfiProxy
    {
        std::string name;
        std::vector<FfiProxyLeg> legs;
        bool udp_supported{};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FfiProxy, name, legs, udp_supported);

    std::string ExtractProxyNameFromCbor(std::span<uint8_t const> cbor);
    std::optional<std::pair<std::string, std::vector<uint8_t>>> ConvertShareLinkToDataProxy(std::string const &link);
    std::optional<std::string> ConvertDataProxyToShareLink(std::string const &name,
                                                           std::span<uint8_t const> data_proxy);
}
