#pragma once

using namespace winrt;
using Windows::Storage::Streams::IBuffer;

namespace winrt::YtFlowApp::implementation
{
    // https://github.com/Microsoft/cppwinrt/issues/189#issuecomment-299304184
    struct __declspec(uuid("905a0fef-bc53-11df-8c49-001e4fc686da")) IBufferByteAccess : ::IUnknown
    {
        virtual HRESULT __stdcall Buffer(void **value) = 0;
    };
    struct VectorBuffer : implements<VectorBuffer, IBuffer, IBufferByteAccess>
    {
        std::vector<uint8_t> m_buffer;
        uint32_t m_length{};

        VectorBuffer(std::vector<uint8_t> buffer) : m_buffer(buffer)
        {
        }

        uint32_t Capacity() const
        {
            return static_cast<uint32_t>(m_buffer.size());
        }

        uint32_t Length() const
        {
            return m_length;
        }

        void Length(uint32_t value)
        {
            if (value > m_buffer.size())
            {
                throw hresult_invalid_argument();
            }

            m_length = value;
        }

        HRESULT __stdcall Buffer(void **value) final
        {
            *value = m_buffer.data();
            return S_OK;
        }
    };
}
