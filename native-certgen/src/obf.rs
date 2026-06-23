// String obfuscation (XOR). Keeps the config base path out of `strings` on the .so.
// NOTE: this is obfuscation only, not real encryption: the key and decoder ship
// inside this binary and can be recovered by a determined reverse engineer.

const K: [u8; 16] = [
    0x4B, 0x39, 0x78, 0x23, 0x6D, 0x50, 0x32, 0x24, 0x76, 0x4C, 0x37, 0x6E, 0x51, 0x34, 0x77, 0x5A,
];

fn dx(data: &[u8]) -> String {
    data.iter()
        .enumerate()
        .map(|(i, b)| (b ^ K[i % K.len()]) as char)
        .collect()
}

/// Decoded base config dir.
pub fn base() -> String {
    dx(&[
        0x64, 0x5D, 0x19, 0x57, 0x0C, 0x7F, 0x5F, 0x4D, 0x05, 0x2F, 0x18, 0x1A, 0x39, 0x51, 0x28,
        0x34, 0x2E, 0x41, 0x0C,
    ])
}
