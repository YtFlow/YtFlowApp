# YtFlowApp
[![Build Status](https://dev.azure.com/YtFlow/YtFlowApp/_apis/build/status/YtFlow.YtFlowApp?branchName=master)](https://dev.azure.com/YtFlow/YtFlowApp/_build/latest?definitionId=1&branchName=master) [![Telegram channel](https://img.shields.io/badge/Telegram-channel-blue)](https://t.me/YtFlowChannel) [![Microsoft Store link](https://img.shields.io/badge/Microsoft-Store-green)](https://www.microsoft.com/store/apps/9NMB8FHL7MBZ)

YtFlow is a network debug tool running on Universal Windows Platform that forwards any network traffic through certain types of tunnels.

## Supported protocols
- Shadowsocks
  - stream ciphers (rc4-md5, aes-128-cfb, aes-192-cfb, aes-256-cfb, aes-128-ctr, aes-192-ctr, aes-256-ctr, camellia-128-cfb, camellia-192-cfb, camellia-256-cfb, salsa20, chacha20, chacha20-ietf)
  - AEAD ciphers (aes-128-gcm, aes-192-gcm, aes-256-gcm, chacha20-poly1305, chacha20-ietf-poly1305, xchacha20-ietf-poly1305)
- Trojan
  - Opt-out of certificate check
- HTTP
  - `CONNECT`-based proxy

## Supported features
### Local SOCKS5 proxy
A SOCKS5 server is available when the VPN connection is on. This is useful for the apps, like Unigram, that dial their server without initiating a DNS lookup and consequently bypass YtFlow VPN tunnel.

Set up SOCKS5 proxies with the following arguments:

| Name | Value |
| ---------------- | ------------------ |
| Server address | `172.17.255.240` |
| Server port | `1080` |
| Authentication method | `none` |
| User name | \<empty\> |
| Password | \<empty\> |

### Debug UDP client
A UDP client is built in for debug purposes. Refer to  [wiki documentation](https://github.com/YtFlow/YtFlowApp/wiki/Remote-debug) for detailed illustration of its usage.

Note that packet capture is **not** enabled for the released packages due to high performance downgrading. To enable packet capture, please manually append `YTLOG_VERBOSE` to conditional compilation symbols of `YtFlowTunnel` and rebuild.

## Limitations
- Trojan via TLS 1.3
- Direct IP connections bypass the proxy
- On Windows builds before v10.0.16299 (including all Mobile versions), a VPN profile must be created manually with the following arguments:

| Name | Value |
| -------------- | ---------- |
| VPN Provider | `YtFlow` |
| Connection name | <code>YtFlow <strong>Auto</strong></code> |
| Server name or address | any |
