use std::fs;
use std::path::Path;

const VERBOSE_MARKER: &str = "/data/misc/the_next/.verbose";

pub fn is_verbose() -> bool {
    Path::new(VERBOSE_MARKER).exists()
}

pub fn set_verbose_marker(enabled: bool) -> Result<(), Box<dyn std::error::Error>> {
    if enabled {
        if let Some(parent) = Path::new(VERBOSE_MARKER).parent() {
            fs::create_dir_all(parent)?;
        }
        fs::write(VERBOSE_MARKER, "")?;
    } else if Path::new(VERBOSE_MARKER).exists() {
        fs::remove_file(VERBOSE_MARKER)?;
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
