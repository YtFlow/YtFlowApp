#pragma once
#include "RawEditorParam.g.h"

namespace winrt::YtFlowApp::implementation
{
    struct RawEditorParam : RawEditorParamT<RawEditorParam>
    {
        RawEditorParam() = default;
        RawEditorParam(uint8_t const *cbor, size_t len);

        hstring RawJson();
        void RawJson(hstring const &value);
        com_array<uint8_t> ToCbor();
        com_array<hstring> CheckErrors();
        void Prettify();
        static void EscapeCborBuf(nlohmann::json &doc);
        static void UnescapeCborBuf(nlohmann::json &doc);

      private:
        hstring m_rawJson;
    };
}
