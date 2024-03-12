#![allow(non_snake_case)]

#[cfg(target_vendor = "uwp")]
extern crate ytflow_app_util;

#[cfg(target_vendor = "uwp")]
mod error;
#[cfg(target_vendor = "uwp")]
mod storage_resource_loader;
#[cfg(target_vendor = "uwp")]
mod tun_plugin;
#[cfg(target_vendor = "uwp")]
mod vpn_plugin;

/// Creates a new instance of Windows.Networking.Vpn.IVpnPlugIn.
/// Returns 0 on success, otherwise failure.
#[no_mangle]
#[cfg(target_vendor = "uwp")]
extern "C" fn CreateVpnPlugIn(plugin_out: *mut *mut std::ffi::c_void) -> i32 {
    let default_hook = std::panic::take_hook();
    std::panic::set_hook(Box::new(move |info| {
        ytflow::log::debug_log(format!("PANIC: {:?}", info));
        default_hook(info);
    }));
    use windows::Networking::Vpn::IVpnPlugIn;

    let plugin: IVpnPlugIn = crate::vpn_plugin::VpnPlugIn::new().into();
    unsafe { *plugin_out = std::mem::transmute(plugin) };
    0
}
