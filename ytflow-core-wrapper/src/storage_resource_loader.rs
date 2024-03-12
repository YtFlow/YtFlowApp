use std::fs;
use std::io;
use std::os::windows::io::FromRawHandle;

use windows::core::ComInterface;
use windows::Storage::StorageFolder;
use windows::Win32::System::WinRT::Storage::{
    IStorageItemHandleAccess, HANDLE_ACCESS_OPTIONS, HANDLE_OPTIONS, HANDLE_SHARING_OPTIONS,
};
use ytflow::resource::{FileResourceLoader, ResourceError, ResourceResult};

const HAO_READ: HANDLE_ACCESS_OPTIONS = HANDLE_ACCESS_OPTIONS(0x120089);
const HSO_SHARE_READ: HANDLE_SHARING_OPTIONS = HANDLE_SHARING_OPTIONS(0x1);
const HO_NONE: HANDLE_OPTIONS = HANDLE_OPTIONS(0);

pub(crate) struct StorageResourceLoader {
    pub(crate) root: StorageFolder,
}

// 😕
unsafe impl Send for StorageResourceLoader {}
unsafe impl Sync for StorageResourceLoader {}

fn hresult_to_resource(r: windows::core::Error) -> ResourceError {
    ResourceError::IoError(io::Error::from_raw_os_error(r.code().0))
}

impl FileResourceLoader for StorageResourceLoader {
    fn load_file(&self, local_name: &str) -> ResourceResult<fs::File> {
        let storage_file = self
            .root
            .GetFileAsync(&local_name.into())
            .map_err(hresult_to_resource)?
            .get()
            .map_err(hresult_to_resource)?;
        let handle_access: IStorageItemHandleAccess = storage_file.cast().unwrap();
        unsafe {
            let handle = handle_access
                .Create(HAO_READ, HSO_SHARE_READ, HO_NONE, None)
                .map_err(hresult_to_resource)?;
            let file = fs::File::from_raw_handle(handle.0 as _);
            Ok(file)
        }
    }
}
