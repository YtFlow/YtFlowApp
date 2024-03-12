use std::cell::{RefCell, UnsafeCell};
use std::ffi::OsString;
use std::mem::ManuallyDrop;
use std::net::{Ipv4Addr, SocketAddr, SocketAddrV4, UdpSocket};
use std::os::windows::ffi::OsStringExt;
use std::rc::Rc;
use std::slice::from_raw_parts_mut;
use std::string::ToString;
use std::sync::{Arc, OnceLock};

use crate::error::ConnectError;
use crate::storage_resource_loader::StorageResourceLoader;

use ytflow::resource::DbFileResourceLoader;
use ytflow::tokio;

use flume::{bounded, Receiver, Sender, TryRecvError};
use windows::core::{h, ComInterface, IInspectable, Result, HSTRING};
use windows::Foundation::Collections::{IIterable, IVectorView};
use windows::Networking::HostName;
use windows::Networking::Sockets::DatagramSocket;
use windows::Networking::Vpn::{
    IVpnPlugIn_Impl, VpnChannel, VpnDomainNameAssignment, VpnDomainNameInfo, VpnDomainNameType,
    VpnPacketBuffer, VpnPacketBufferList, VpnRoute, VpnRouteAssignment,
};
use windows::Storage::Streams::Buffer;
use windows::Storage::{ApplicationData, ApplicationDataContainer, CreationCollisionOption};
use windows::Win32::System::WinRT::IBufferByteAccess;
use windows_implement::implement;

static APP_SETTINGS: OnceLock<ApplicationDataContainer> = OnceLock::new();

/// Safety: user must ensure the output slice does not outlive the buffer instance.
pub(crate) unsafe fn query_slice_from_ibuffer_mut(buf: &mut Buffer) -> &'static mut [u8] {
    let len = buf.Capacity().unwrap() as _;
    let byte_access: IBufferByteAccess = buf.cast().unwrap();
    #[allow(unused_unsafe)]
    unsafe {
        let ptr = byte_access.Buffer().unwrap();
        from_raw_parts_mut(ptr, len)
    }
}

fn connect_with_factory(
    transport: &DatagramSocket,
    factory: &ytflow::config::plugin::VpnTunFactory,
    channel: &VpnChannel,
) -> Result<()> {
    let ipv4 = factory
        .ipv4
        .as_ref()
        .map(ToString::to_string)
        .map(|s| HostName::CreateHostName(&s.into()))
        .transpose()?
        .map(|h| IVectorView::try_from(vec![Some(h)]))
        .transpose()?;
    let ipv6 = factory
        .ipv6
        .as_ref()
        .map(ToString::to_string)
        .map(|s| HostName::CreateHostName(&s.into()))
        .transpose()?
        .map(|h| IVectorView::try_from(vec![Some(h)]))
        .transpose()?;

    let route_scope = VpnRouteAssignment::new()?;
    route_scope.SetExcludeLocalSubnets(true)?;
    // Adding routes without IP addr being set will cause the VPN platform to hang.
    if ipv4.is_some() {
        for route4 in &factory.ipv4_route {
            route_scope
                .Ipv4InclusionRoutes()?
                .Append(&VpnRoute::CreateVpnRoute(
                    &HostName::CreateHostName(&route4.inner.first_address().to_string().into())?,
                    route4.inner.network_length(),
                )?)?;
        }
    }
    if ipv6.is_some() {
        for route6 in &factory.ipv6_route {
            route_scope
                .Ipv6InclusionRoutes()?
                .Append(&VpnRoute::CreateVpnRoute(
                    &HostName::CreateHostName(&route6.inner.first_address().to_string().into())?,
                    route6.inner.network_length(),
                )?)?;
        }
    }

    let dns_assignments = VpnDomainNameAssignment::new()?;
    let dns_hosts: Result<Vec<_>> = factory
        .dns
        .iter()
        .map(ToString::to_string)
        .map(|s| HostName::CreateHostName(&s.into()).map(Some))
        .collect();
    let proxy: Result<Vec<_>> = factory
        .web_proxy
        .iter()
        .map(|s| HostName::CreateHostName(&s.into()).map(Some))
        .collect();
    let dnsinfo = VpnDomainNameInfo::CreateVpnDomainNameInfo(
        h!("."),
        VpnDomainNameType::Suffix,
        &IIterable::<HostName>::try_from(dns_hosts?)?,
        &IIterable::<HostName>::try_from(proxy?)?,
    )?;
    dns_assignments.DomainNameList()?.Append(&dnsinfo)?;

    channel.StartWithMainTransport(
        ipv4.as_ref(),
        ipv6.as_ref(),
        None,
        &route_scope,
        &dns_assignments,
        1512,
        3,
        false,
        transport,
    )
}

async fn run_rpc(control_hub: ytflow::control::ControlHub) -> std::io::Result<()> {
    use tokio::net::TcpSocket;
    use ytflow::control::rpc::{serve_stream, ControlHubService};
    let s = TcpSocket::new_v4()?;
    s.set_reuseaddr(true)?;
    s.bind(SocketAddr::new(Ipv4Addr::LOCALHOST.into(), 0))?;
    let port = HSTRING::from(s.local_addr()?.port().to_string());
    let _ = APP_SETTINGS.get().unwrap().Values()?.Insert(
        h!("YTFLOW_CORE_RPC_PORT"),
        &IInspectable::try_from(port).unwrap(),
    );
    let listener = s.listen(128)?;
    let task = || async {
        use tokio::time::{sleep, Duration};
        let mut service = ControlHubService(&control_hub);
        loop {
            let (stream, _) = match listener.accept().await {
                Ok(s) => s,
                _ => {
                    // retry listening
                    // TODO: log errors
                    sleep(Duration::from_secs(1)).await;
                    continue;
                }
            };
            stream.set_nodelay(true)?;
            let _ = serve_stream(&mut service, stream).await;
        }
        #[allow(unreachable_code)]
        Ok::<_, std::io::Error>(())
    };
    let _ = futures::future::join(task(), task()).await;
    Ok(())
}

struct Runtime {
    rx_buf_tx: ManuallyDrop<Sender<Vec<u8>>>,
    plugin_set: ManuallyDrop<ytflow::config::PluginSet>,
    rt: tokio::runtime::Runtime,
    rpc_task: tokio::task::JoinHandle<std::io::Result<()>>,
}

impl Drop for Runtime {
    fn drop(&mut self) {
        let _enter_guard = self.rt.enter();
        unsafe {
            self.rpc_task.abort();
            let _rx_buf_tx = ManuallyDrop::take(&mut self.rx_buf_tx);
            let _plugin_set = ManuallyDrop::take(&mut self.plugin_set);
        }
    }
}

#[derive(Default)]
enum VpnPlugInInner {
    #[default]
    NotRunning,
    Running {
        tx_buf_rx: Receiver<VpnPacketBuffer>,
        runtime: Runtime,
    },
    Stopping {
        tx_buf_rx: Receiver<VpnPacketBuffer>,
    },
}

impl VpnPlugInInner {
    fn request_stop(&mut self) {
        let this = std::mem::take(self);
        *self = match this {
            Self::Running {
                tx_buf_rx,
                runtime: _, // Drop the runtime.
            } => Self::Stopping { tx_buf_rx },
            _ => this,
        }
    }
}

#[implement(windows::Networking::Vpn::IVpnPlugIn)]
pub struct VpnPlugIn(UnsafeCell<VpnPlugInInner>);

#[allow(non_snake_case)]
impl VpnPlugIn {
    pub fn new() -> Self {
        let _ = APP_SETTINGS.set(ApplicationData::Current().unwrap().LocalSettings().unwrap());
        Self(Default::default())
    }

    fn connect_core(&self, channel: &VpnChannel) -> std::result::Result<(), ConnectError> {
        let inner = unsafe { &mut *self.0.get() };
        *inner = Default::default();

        let transport = DatagramSocket::new()?;
        channel.AssociateTransport(&transport, None)?;
        let lo_host = HostName::CreateHostName(h!("127.0.0.1"))?;
        transport
            .BindEndpointAsync(&lo_host, &HSTRING::new())?
            .get()?;
        let b_transport =
            UdpSocket::bind(SocketAddr::V4(SocketAddrV4::new(Ipv4Addr::LOCALHOST, 0))).unwrap();
        transport
            .ConnectAsync(
                &lo_host.clone(),
                &b_transport.local_addr().unwrap().port().to_string().into(),
            )?
            .get()?;
        let transport_port = transport
            .Information()?
            .LocalPort()?
            .to_string()
            .parse()
            .unwrap();
        b_transport
            .connect(SocketAddr::V4(SocketAddrV4::new(
                Ipv4Addr::LOCALHOST,
                transport_port,
            )))
            .unwrap();

        // Load VPN configurations.
        let db_path: std::path::PathBuf = OsString::from_wide(
            HSTRING::try_from(
                APP_SETTINGS
                    .get()
                    .unwrap()
                    .Values()?
                    .Lookup(h!("YTFLOW_DB_PATH"))?,
            )?
            .as_wide(),
        )
        .into();
        let db = match ytflow::data::Database::open(db_path) {
            Ok(db) => db,
            Err(e) => return Err(format!("Cannot open database: {}", e).into()),
        };
        let conn = match db.connect() {
            Ok(conn) => conn,
            Err(e) => return Err(format!("Cannot connect to database: {}", e).into()),
        };

        fn load_plugins(
            profile_id: usize,
            conn: &ytflow::data::Connection,
        ) -> ytflow::data::DataResult<
            std::result::Result<(Vec<ytflow::config::Plugin>, Vec<ytflow::config::Plugin>), String>,
        > {
            use ytflow::data::{Plugin, Profile};
            let profile_id = match Profile::query_by_id(profile_id, conn)? {
                Some(p) => p.id,
                None => return Ok(Err(format!("Profile {} not found", profile_id))),
            };
            let entry_plugins = Plugin::query_entry_by_profile(profile_id, conn)?
                .into_iter()
                .map(From::from)
                .collect();
            let all_plugins = Plugin::query_all_by_profile(profile_id, conn)?
                .into_iter()
                .map(From::from)
                .collect();
            Ok(Ok((entry_plugins, all_plugins)))
        }

        let profile_id: u32 = APP_SETTINGS
            .get()
            .unwrap()
            .Values()?
            .Lookup(h!("YTFLOW_PROFILE_ID"))?
            .try_into()
            .unwrap_or(0);
        let (entry_plugins, all_plugins) = match load_plugins(profile_id as _, &conn) {
            Ok(Ok(p)) => p,
            Ok(Err(s)) => return Err(s.into()),
            Err(e) => return Err(format!("Failed to load plugins from: {}", e).into()),
        };

        use ytflow::config::loader::{ProfileLoadResult, ProfileLoader};
        let (factory, required_resources, errors) =
            ProfileLoader::parse_profile(entry_plugins.iter(), &all_plugins);
        if !errors.is_empty() {
            let it = std::iter::once(String::from("Failed to parse plugins: "));
            let it = it.chain(errors.iter().map(ToString::to_string));
            let strs: Vec<_> = it.collect();
            return Err(strs.join("\r\n").into());
        }

        let rt = match tokio::runtime::Runtime::new() {
            Ok(rt) => rt,
            Err(e) => return Err(format!("Cannot create tokio runtime: {}", e).into()),
        };

        let vpn_items_cell = Rc::new(RefCell::new(None));
        ytflow::config::plugin::ON_VPNTUN.with(|cb| {
            let vpn_items_cell = vpn_items_cell.clone();
            let channel = channel.clone();
            *cb.borrow_mut() = Some(Box::new(move |f| {
                // TODO: bounded? capacity?
                let (tx_buf_tx, tx_buf_rx) = bounded(16);
                let (rx_buf_tx, rx_buf_rx) = bounded::<Vec<u8>>(16);
                *vpn_items_cell.borrow_mut() = Some((tx_buf_rx, rx_buf_tx, f.clone()));

                Arc::new(super::tun_plugin::VpnTun::new(
                    channel,
                    tx_buf_tx,
                    rx_buf_rx,
                    b_transport,
                ))
            }));
        });

        let rt_handle = rt.handle();
        let ProfileLoadResult {
            plugin_set: set,
            errors,
            control_hub,
        } = {
            let conn = db.connect()?;
            let resource_keys = required_resources
                .iter()
                .map(|r| r.key.to_string())
                .collect();
            let loader = DbFileResourceLoader::new_with_required_keys(resource_keys, &conn)?;
            let resource_folder = ApplicationData::Current()
                .unwrap()
                .LocalFolder()
                .unwrap()
                .CreateFolderAsync(h!("resource"), CreationCollisionOption::OpenIfExists)
                .unwrap()
                .get()
                .unwrap();
            let loader = rt_handle.block_on(async move {
                let mut loader = loader;
                let res =
                    futures::future::join_all(loader.load_required_files(&StorageResourceLoader {
                        root: resource_folder,
                    }))
                    .await
                    .into_iter()
                    .collect::<std::result::Result<Vec<_>, _>>();
                res.map(|_| loader)
            })?;
            factory.load_all(rt_handle, Box::new(loader), Some(&db))
        };
        let mut error_str = if errors.is_empty() {
            String::from("There must be exactly one vpn-tun entry plugin in a profile")
        } else {
            let it = std::iter::once(String::from("Failed to instantiate plugins: "));
            let it = it.chain(errors.iter().map(ToString::to_string));
            it.collect::<Vec<_>>().join("\r\n")
        };

        loop {
            if let (Some(vpn_items), []) = (vpn_items_cell.borrow_mut().take(), &*errors) {
                let (tx_buf_rx, rx_buf_tx, vpn_tun_factory) = vpn_items;
                match connect_with_factory(&transport, &vpn_tun_factory, channel) {
                    Ok(()) => {
                        *inner = VpnPlugInInner::Running {
                            tx_buf_rx,
                            runtime: Runtime {
                                rx_buf_tx: ManuallyDrop::new(rx_buf_tx),
                                plugin_set: ManuallyDrop::new(set),
                                rpc_task: rt_handle.spawn(run_rpc(control_hub)),
                                rt,
                            },
                        };
                        break Ok(());
                    }
                    Err(e) => {
                        error_str += &format!("\r\nFailed to Connect VPN Tunnel: {}", e);
                        continue;
                    }
                }
            } else {
                {
                    let _enter_guard = rt_handle.enter();
                    drop(set);
                }
                rt.shutdown_background();
                ytflow::config::plugin::ON_VPNTUN.with(|cb| drop(cb.borrow_mut().take()));
                break Err(error_str.into());
            };
        }
    }
}

impl IVpnPlugIn_Impl for VpnPlugIn {
    fn Connect(&self, channel: Option<&VpnChannel>) -> Result<()> {
        let channel = channel.unwrap();
        if let Err(crate::error::ConnectError(err_msg)) = self.connect_core(channel) {
            APP_SETTINGS.get().unwrap().Values()?.Insert(
                h!("YTFLOW_CORE_ERROR_LOAD"),
                &IInspectable::try_from(HSTRING::from(&err_msg))?,
            )?;
            channel.TerminateConnection(&err_msg.into())?;
        }
        Ok(())
    }
    fn Disconnect(&self, _channel: Option<&VpnChannel>) -> Result<()> {
        // Don't .Stop() the channel because there may be some inflight tx buffers remaining,
        // which will block the VPN background task, leading to the whole process being killed.
        // .Stop() until all buffers are drained in Decapsulate.
        unsafe {
            (*self.0.get()).request_stop();
        }
        Ok(())
    }
    fn GetKeepAlivePayload(
        &self,
        _channel: Option<&VpnChannel>,
        keepAlivePacket: &mut Option<VpnPacketBuffer>,
    ) -> Result<()> {
        *keepAlivePacket = None;
        Ok(())
    }
    fn Encapsulate(
        &self,
        _channel: Option<&VpnChannel>,
        packets: Option<&VpnPacketBufferList>,
        _encapulatedPackets: Option<&VpnPacketBufferList>,
    ) -> Result<()> {
        let packets = packets.unwrap();
        let rx_buf_tx = if let VpnPlugInInner::Running { runtime, .. } = unsafe { &*self.0.get() } {
            &runtime.rx_buf_tx
        } else {
            return Ok(());
        };
        let packet_count = packets.Size()?;
        for _ in 0..packet_count {
            let vpn_buffer = packets.RemoveAtBegin()?;
            let mut buffer = vpn_buffer.Buffer()?;
            let len = buffer.Capacity().unwrap() as _;
            let slice = unsafe { query_slice_from_ibuffer_mut(&mut buffer) };
            let mut buf = Vec::with_capacity(len);
            unsafe {
                std::ptr::copy_nonoverlapping(slice.as_mut_ptr(), buf.as_mut_ptr(), len);
                buf.set_len(len);
            }
            if rx_buf_tx.send(buf).is_err() {
                return Ok(());
            }
            packets.Append(&vpn_buffer)?;
        }
        Ok(())
    }
    fn Decapsulate(
        &self,
        channel: Option<&VpnChannel>,
        _encapBuffer: Option<&VpnPacketBuffer>,
        decapsulatedPackets: Option<&VpnPacketBufferList>,
        _controlPacketsToSend: Option<&VpnPacketBufferList>,
    ) -> Result<()> {
        let inner = unsafe { &mut *self.0.get() };

        let decapsulatedPackets = decapsulatedPackets.unwrap();
        let tx_buf_rx = match inner {
            VpnPlugInInner::NotRunning => return Ok(()),
            VpnPlugInInner::Running { tx_buf_rx, .. } => tx_buf_rx,
            VpnPlugInInner::Stopping { tx_buf_rx } => tx_buf_rx,
        };
        let mut idle_loop_count = 0;
        loop {
            match tx_buf_rx.try_recv() {
                Ok(buf) => {
                    idle_loop_count = 0;
                    decapsulatedPackets.Append(&buf)?;
                }
                Err(TryRecvError::Disconnected) => {
                    *inner = VpnPlugInInner::NotRunning;
                    let _ = channel.unwrap().Stop();
                    return Ok(());
                }
                Err(TryRecvError::Empty) if idle_loop_count < 8 => {
                    idle_loop_count += 1;
                    continue;
                }
                Err(TryRecvError::Empty) => break,
            }
        }
        Ok(())
    }
}
