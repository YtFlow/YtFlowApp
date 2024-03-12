use std::mem::transmute;
use std::sync::{Arc, OnceLock};

use ytflow::flow::*;

use flume::{Receiver, Sender, TrySendError};
use windows::Networking::Vpn::{VpnChannel, VpnPacketBuffer};
use windows::Storage::Streams::Buffer as NativeBuffer;

unsafe fn token_to_native_buffer(token: TunBufferToken) -> (VpnPacketBuffer, NativeBuffer) {
    let ([vpn_buffer_ptr, buffer_ptr], _) = token.into_parts();
    (transmute(vpn_buffer_ptr), transmute(buffer_ptr))
}

struct SenderContext {
    tx: Sender<VpnPacketBuffer>,
    dummy_socket: std::net::UdpSocket,
}

pub(super) struct VpnTun {
    channel: VpnChannel,
    rx: Receiver<Buffer>,
    tx_ctx: Arc<SenderContext>,
}

impl SenderContext {
    fn send_buffer(&self, vpn_buffer: VpnPacketBuffer) {
        if let Err(TrySendError::Full(vpn_buffer)) = self.tx.try_send(vpn_buffer) {
            // TODO: intentionally block?
            // VpnPacketBuffer must be returned back to system to dealloc
            let _ = self.dummy_socket.send(&[1][..]);
            let _ = self.tx.send(vpn_buffer);
        }
    }
    fn flush(&self) {
        let _ = self.dummy_socket.send(&[1][..]);
    }
    fn flush_send_buffer(&self, vpn_buffer: VpnPacketBuffer) {
        self.send_buffer(vpn_buffer);
        if self.tx.len() == 1 {
            self.flush();
        }
    }
}

impl VpnTun {
    pub fn new(
        channel: VpnChannel,
        tx_buf_tx: Sender<VpnPacketBuffer>,
        rx_buf_rx: Receiver<Buffer>,
        dummy_socket: std::net::UdpSocket,
    ) -> Self {
        Self {
            channel,
            rx: rx_buf_rx,
            tx_ctx: Arc::new(SenderContext {
                tx: tx_buf_tx,
                dummy_socket,
            }),
        }
    }
    fn send_buffer_direct(&self, vpn_buffer: VpnPacketBuffer) {
        self.tx_ctx.flush_send_buffer(vpn_buffer);
    }
    fn send_buffer_worker(&self, vpn_buffer: VpnPacketBuffer) {
        static WORK_TX: OnceLock<std::sync::mpsc::Sender<(Arc<SenderContext>, VpnPacketBuffer)>> =
            OnceLock::new();
        let work_tx = WORK_TX.get_or_init(|| {
            let (tx, rx) = std::sync::mpsc::channel();
            std::thread::spawn(move || {
                let mut tasks: Vec<(Arc<SenderContext>, VpnPacketBuffer)> = vec![];
                loop {
                    tasks.clear();
                    if let Ok(task) = rx.recv() {
                        tasks.push(task);
                        while let Ok(task) = rx.try_recv() {
                            tasks.push(task);
                            if tasks.len() > 8 {
                                break;
                            }
                        }
                    } else {
                        return;
                    }

                    let mut last_tx_ctx = None;
                    for (tx_ctx, packet) in tasks.drain(..) {
                        // tx_ctx.flush_send_buffer(packet);
                        tx_ctx.send_buffer(packet);
                        last_tx_ctx = Some(tx_ctx);
                    }
                    if let Some(tx_ctx) = last_tx_ctx {
                        tx_ctx.flush();
                    }
                }
            });
            tx
        });
        work_tx.send((self.tx_ctx.clone(), vpn_buffer)).ok();
    }
}

impl Tun for VpnTun {
    // Read
    fn blocking_recv(&self) -> Option<Buffer> {
        self.rx.recv().ok()
    }
    fn return_recv_buffer(&self, _buf: Buffer) {}

    // Write
    fn get_tx_buffer(&self) -> Option<TunBufferToken> {
        let vpn_buffer = self.channel.GetVpnReceivePacketBuffer().ok()?;
        let mut buffer = vpn_buffer.Buffer().ok()?;
        Some(unsafe {
            let data = crate::vpn_plugin::query_slice_from_ibuffer_mut(&mut buffer);
            TunBufferToken::new([transmute(vpn_buffer), transmute(buffer)], data)
        })
    }
    fn send(&self, buf: TunBufferToken, len: usize) {
        let (vpn_buffer, buffer) = unsafe { token_to_native_buffer(buf) };
        // In case SetLength fails, try to consume the invalid packet as well to prevent leak.
        let _ = buffer.SetLength(len as u32);
        self.send_buffer_worker(vpn_buffer);
    }
    fn return_tx_buffer(&self, buf: TunBufferToken) {
        let (vpn_buffer, _) = unsafe { token_to_native_buffer(buf) };
        // Try to consume the potentially invalid packet to prevent leak.
        self.send_buffer_direct(vpn_buffer);
    }
}

impl Drop for SenderContext {
    fn drop(&mut self) {
        // Signal to Decapsulate to drain all tx buffers (if any) and shutdown the channel.
        let _ = self.dummy_socket.send(&[1][..]);
    }
}
