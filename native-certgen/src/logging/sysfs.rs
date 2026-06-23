use std::fs;
use std::path::Path;

fn marker() -> String {
    format!("{}/.verbose", crate::obf::base())
}

pub fn is_verbose() -> bool {
    Path::new(&marker()).exists()
}

pub fn set_verbose_marker(enabled: bool) -> Result<(), Box<dyn std::error::Error>> {
    let m = marker();
    if enabled {
        if let Some(parent) = Path::new(&m).parent() {
            fs::create_dir_all(parent)?;
        }
        fs::write(&m, "")?;
    } else if Path::new(&m).exists() {
        fs::remove_file(&m)?;
    }
    Ok(())
}

pub fn enable() -> Result<(), Box<dyn std::error::Error>> {
    set_verbose_marker(true)?;
    tracing::info!("verbose logging enabled via marker file");
    Ok(())
}

pub fn disable() -> Result<(), Box<dyn std::error::Error>> {
    set_verbose_marker(false)?;
    tracing::info!("verbose logging disabled, marker file removed");
    Ok(())
}

pub fn status() -> Result<(), Box<dyn std::error::Error>> {
    let state = if is_verbose() { "enabled" } else { "disabled" };
    tracing::info!(verbose = state, "verbose marker status");
    Ok(())
}
