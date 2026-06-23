use std::fs::{self, File};
use std::io::{Read, Write};
use std::path::Path;
use std::process::Command;
use std::time::{SystemTime, UNIX_EPOCH};

const DUMP_DIR: &str = "/sdcard/Download";
const LOGCAT_SIZE_LIMIT: usize = 2 * 1024 * 1024;

struct FlockGuard {
    _file: File,
}

impl FlockGuard {
    fn acquire() -> Result<Self, Box<dyn std::error::Error>> {
        let lock_path = format!("{}/.dump_lock", crate::obf::base());
        if let Some(parent) = Path::new(&lock_path).parent() {
            fs::create_dir_all(parent)?;
        }
        let file = File::create(&lock_path)?;
        let fd = {
            use std::os::unix::io::AsRawFd;
            file.as_raw_fd()
        };
        let ret = unsafe { libc::flock(fd, libc::LOCK_EX | libc::LOCK_NB) };
        if ret != 0 {
            return Err("dump already in progress".into());
        }
        Ok(Self { _file: file })
    }
}

impl Drop for FlockGuard {
    fn drop(&mut self) {
        // flock released automatically when file descriptor closes
    }
}

fn random_name(len: usize) -> String {
    use rand::Rng;
    let mut rng = rand::thread_rng();
    (0..len)
        .map(|_| {
            let idx = rng.gen_range(0..36u8);
            if idx < 10 {
                (b'0' + idx) as char
            } else {
                (b'a' + idx - 10) as char
            }
        })
        .collect()
}

fn collect_logcat(tag: &str) -> Vec<u8> {
    let output = Command::new("logcat")
        .args(["-d", "-s", tag])
        .output();

    match output {
        Ok(o) => {
            let mut data = o.stdout;
            data.truncate(LOGCAT_SIZE_LIMIT);
            data
        }
        Err(_) => Vec::new(),
    }
}

fn collect_device_info() -> String {
    let mut info = String::new();

    if let Ok(output) = Command::new("uname").arg("-a").output() {
        info.push_str(&format!(
            "uname={}\n",
            String::from_utf8_lossy(&output.stdout).trim()
        ));
    }

    for (key, prop) in [
        ("device", "ro.product.device"),
        ("build", "ro.build.display.id"),
        ("android", "ro.build.version.release"),
    ] {
        if let Ok(output) = Command::new("getprop").arg(prop).output() {
            info.push_str(&format!(
                "{}={}\n",
                key,
                String::from_utf8_lossy(&output.stdout).trim()
            ));
        }
    }

    // KSU version
    if let Ok(ver) = fs::read_to_string("/data/adb/ksu/version") {
        info.push_str(&format!("ksu={}\n", ver.trim()));
    }

    // Module version from module.prop
    if let Ok(prop) = fs::read_to_string("/data/adb/modules/integrityfateh7/module.prop") {
        for line in prop.lines() {
            if let Some(ver) = line.strip_prefix("version=") {
                info.push_str(&format!("module={}\n", ver.trim()));
                break;
            }
        }
    }

    info
}

fn read_file_bytes(path: &str) -> Option<Vec<u8>> {
    let mut buf = Vec::new();
    File::open(path).ok()?.read_to_end(&mut buf).ok()?;
    Some(buf)
}

fn epoch_millis() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_millis() as u64)
        .unwrap_or(0)
}

pub fn execute_dump() -> Result<(), Box<dyn std::error::Error>> {
    let _lock = FlockGuard::acquire()?;

    let _ = fs::create_dir_all(DUMP_DIR);
    let zip_name = format!("{}.zip", random_name(8));
    let zip_path = format!("{}/{}", DUMP_DIR, zip_name);

    let zip_file = File::create(&zip_path)?;
    let mut zip = zip::ZipWriter::new(zip_file);
    let options =
        zip::write::SimpleFileOptions::default().compression_method(zip::CompressionMethod::Deflated);

    let mut file_count = 0u32;

    // Log files
    let log_files = [
        "certgen.log",
        "certgen.log.1",
        "certgen.log.2",
        "certgen.log.3",
        "certgen.log.4",
    ];
    for name in &log_files {
        let path = format!("{}/logs/{}", crate::obf::base(), name);
        if let Some(data) = read_file_bytes(&path) {
            zip.start_file(*name, options)?;
            zip.write_all(&data)?;
            file_count += 1;
        }
    }

    // Logcat
    let logcat = collect_logcat("TEESimulator");
    if !logcat.is_empty() {
        zip.start_file("logcat-teesimulator.log", options)?;
        zip.write_all(&logcat)?;
        file_count += 1;
    }

    // Config files
    for name in ["tee_status.txt", "security_patch.txt"] {
        let path = format!("{}/{}", crate::obf::base(), name);
        if let Some(data) = read_file_bytes(&path) {
            zip.start_file(name, options)?;
            zip.write_all(&data)?;
            file_count += 1;
        }
    }

    // Device info
    let device_info = collect_device_info();
    if !device_info.is_empty() {
        zip.start_file("device-info.txt", options)?;
        zip.write_all(device_info.as_bytes())?;
        file_count += 1;
    }

    // Manifest
    let manifest = serde_json::json!({
        "timestamp": epoch_millis(),
        "version": env!("CARGO_PKG_VERSION"),
        "files": file_count,
    });
    zip.start_file("manifest.json", options)?;
    zip.write_all(manifest.to_string().as_bytes())?;

    zip.finish()?;

    let zip_size = fs::metadata(&zip_path).map(|m| m.len()).unwrap_or(0);
    fs::write(format!("{}/.dump_path", crate::obf::base()), &zip_path)?;

    let result = serde_json::json!({
        "zip": zip_path,
        "size": zip_size,
        "files": file_count + 1, // +1 for manifest
    });
    println!("{}", result);

    tracing::info!(path = %zip_path, size = zip_size, "diagnostic dump created");

    Ok(())
}
