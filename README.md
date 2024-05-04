# YtFlowApp
[![Build Status](https://dev.azure.com/YtFlow/YtFlowApp/_apis/build/status/YtFlow.YtFlowApp?branchName=next)](https://dev.azure.com/YtFlow/YtFlowApp/_build/latest?definitionId=1&branchName=next) [![Telegram channel and group](https://img.shields.io/badge/Telegram-group-blue)](https://t.me/YtFlow) [![Join Beta testing](https://img.shields.io/badge/Join-Beta%20testing-blue)](https://forms.office.com/Pages/ResponsePage.aspx?id=DQSIkWdsW0yxEjajBLZtrQAAAAAAAAAAAAZAAJskVm9UMThNRlZRR1E5Q1dBMjM1MjFXVEk0UDlTMS4u)

<a href='https://www.microsoft.com/store/apps/9NMB8FHL7MBZ?cid=storebadge&ocid=badge'><img src='https://developer.microsoft.com/en-us/store/badges/images/English_get-it-from-MS.png' alt='English badge' width="240"/></a>

YtFlow is a network debug tool running on Universal Windows Platform that forwards any network traffic through certain types of tunnels, powered by [YtFlowCore](https://github.com/YtFlow/YtFlowCore).

[Join the Beta Testing program](https://forms.office.com/Pages/ResponsePage.aspx?id=DQSIkWdsW0yxEjajBLZtrQAAAAAAAAAAAAZAAJskVm9UMThNRlZRR1E5Q1dBMjM1MjFXVEk0UDlTMS4u) to keep updated with the latest YtFlow Beta versions.

## Supported Features

### Protocols
- [Shadowsocks](https://ytflow.github.io/ytflow-book/plugins/shadowsocks-client.html)
  - stream ciphers (rc4-md5, aes-128-cfb, aes-192-cfb, aes-256-cfb, aes-128-ctr, aes-192-ctr, aes-256-ctr, camellia-128-cfb, camellia-192-cfb, camellia-256-cfb, salsa20, chacha20-ietf)
  - AEAD ciphers (aes-128-gcm, aes-192-gcm, aes-256-gcm, chacha20-ietf-poly1305, xchacha20-ietf-poly1305)
- [Trojan](https://ytflow.github.io/ytflow-book/plugins/trojan-client.html)
- [HTTP](https://ytflow.github.io/ytflow-book/plugins/http-proxy-client.html)
  - `CONNECT`-based proxy
- [SOCKS5](https://ytflow.github.io/ytflow-book/plugins/socks5-client.html)
- [VMess](https://ytflow.github.io/ytflow-book/plugins/vmess-client.html)
  - VMess AEAD header format (`alterID`=0)
  - Customizable WebSocket and TLS transport

### Transports

- [TLS](https://ytflow.github.io/ytflow-book/plugins/tls-client.html)
- [WebSocket](https://ytflow.github.io/ytflow-book/plugins/ws-client.html)
  - Based on HTTP/1.1
- `simple-obfs` compatible obfuscation
  - [http](https://ytflow.github.io/ytflow-book/plugins/http-obfs-client.html)
  - [tls](https://ytflow.github.io/ytflow-book/plugins/tls-obfs-client.html)

### Share Links

- Legacy Shadowsocks format
- Shadowsocks SIP002
- SOCKS5
- HTTP Proxy
- Trojan
- VMess (v2rayN flavor)

### Subscriptions

- Base64-encoded share links
- SIP008
- Surge proxy list

## Getting Started

- [Quick Start](https://github.com/YtFlow/YtFlowApp/wiki/Quick-Start)
- [Configuration Guide](https://github.com/YtFlow/YtFlowApp/wiki/Configuration-Guide)
- [YtFlowCore Book](https://ytflow.github.io/ytflow-book)
