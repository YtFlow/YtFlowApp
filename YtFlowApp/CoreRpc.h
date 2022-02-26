#pragma once

using namespace winrt::Windows::Foundation;

namespace winrt::YtFlowApp::implementation
{
    struct RpcException : public std::exception
    {
        std::string msg;

        const char *what() const throw() override;
    };
    struct RpcPluginInfo
    {
        // https://github.com/ReactiveX/RxCpp/issues/420
        bool operator==(const RpcPluginInfo &) const;

        uint32_t id{};
        std::string name;
        std::string plugin;
        nlohmann::json::binary_t info;
        uint32_t hashcode{};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RpcPluginInfo, id, name, plugin, info, hashcode)
    struct CoreRpc final
    {
        CoreRpc()
        {
        }

        static CoreRpc Connect();

        std::vector<RpcPluginInfo> CollectAllPluginInfo(std::map<uint32_t, uint32_t> const &hashcodes) const &;

        Windows::Networking::Sockets::StreamSocket m_socket{nullptr};

      private:
        CoreRpc(winrt::Windows::Networking::Sockets::StreamSocket socket) : m_socket(socket)
        {
        }

        static std::vector<uint8_t> ReadChunk(Windows::Storage::Streams::IInputStream stream);

        std::shared_ptr<std::mutex> m_ioLock{std::make_shared<std::mutex>()};
    };
}
