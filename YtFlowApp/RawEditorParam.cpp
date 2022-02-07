#include "pch.h"
#include "RawEditorParam.h"
#include "RawEditorParam.g.cpp"

#include "base64.h"
#include <icu.h>
#include <wincrypt.h>

using namespace nlohmann;

namespace winrt::YtFlowApp::implementation
{
    RawEditorParam::RawEditorParam(uint8_t const *cbor, size_t len)
    {
        auto doc{json::from_cbor(cbor, cbor + len)};
        EscapeCborBuf(doc);
        m_rawJson = to_hstring(doc.dump(2));
    }

    hstring RawEditorParam::RawJson()
    {
        return m_rawJson;
    }
    void RawEditorParam::RawJson(hstring const &value)
    {
        m_rawJson = value;
    }
    com_array<uint8_t> RawEditorParam::ToCbor()
    {
        auto doc{json::parse(winrt::to_string(m_rawJson))};
        UnescapeCborBuf(doc);
        return com_array<uint8_t>{json::to_cbor(doc)};
    }
    com_array<hstring> RawEditorParam::CheckErrors()
    {
        try
        {
            auto const _{json::parse(winrt::to_string(m_rawJson))};
        }
        catch (json::parse_error &e)
        {
            std::array<hstring, 1> arr{to_hstring(e.what())};
            return com_array<hstring>(arr);
        }
        return {};
    }
    void RawEditorParam::Prettify()
    {
        auto doc{json::parse(winrt::to_string(m_rawJson))};
        m_rawJson = to_hstring(doc.dump(2));
    }

    void RawEditorParam::EscapeCborBuf(json &doc)
    {
        if (doc.is_array())
        {
            for (auto &item : doc)
            {
                EscapeCborBuf(item);
            }
            return;
        }
        else if (doc.is_object())
        {
            for (auto &[key, val] : doc.items())
            {
                EscapeCborBuf(val);
            }
            return;
        }
        else if (!doc.is_binary())
        {
            return;
        }
        auto const bin{doc.get_binary()};

        size_t offset{0};
        UChar32 outChar{0};
        while (offset < bin.size() && outChar >= 0)
        {
            U8_NEXT(bin.data(), offset, bin.size(), outChar);
        }

        if (outChar >= 0)
        {
            // Valid UTF-8
            doc = std::move("{ \"__byte_repr\": \"utf8\" }"_json);
            doc["data"] = std::string(bin.data(), bin.data() + bin.size());
        }
        else
        {
            // Invalid UTF-8, Base64 it
            doc = std::move("{ \"__byte_repr\": \"base64\" }"_json);
            doc["data"] = base64_encode(std::string(bin.data(), bin.data() + bin.size()));
        }
    }

    void RawEditorParam::UnescapeCborBuf(json &doc)
    {
        if (doc.is_array())
        {
            for (auto &item : doc)
            {
                UnescapeCborBuf(item);
            }
            return;
        }
        else if (!doc.is_object())
        {
            return;
        }

        auto const reprIt{doc.find("__byte_repr")};
        auto const dataIt{doc.find("data")};
        if (reprIt == doc.end() || dataIt == doc.end() || !reprIt.value().is_string() || !dataIt.value().is_string())
        {
            for (auto &[key, val] : doc.items())
            {
                UnescapeCborBuf(val);
            }
            return;
        }

        std::string const repr{reprIt.value()};
        std::string const data{dataIt.value()};
        if (repr == "utf8")
        {
            doc = json::binary(std::vector<uint8_t>(data.data(), data.data() + data.size()));
        }
        else if (repr == "base64")
        {
            std::string decoded{base64_decode(data, true)};
            doc = json::binary(std::vector<uint8_t>(decoded.data(), decoded.data() + decoded.size()));
        }
    }

}
