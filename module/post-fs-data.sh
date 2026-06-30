MODPATH="${0%/*}"
. "$MODPATH/common_func.sh"

# Requires resetprop (Magisk / KSU+susfs / APatch). Exit quietly if missing.
command -v resetprop >/dev/null 2>&1 || exit 0

# Optional hooks if present (kept for forward-compat; harmless if absent).
[ -f "$MODPATH/common_setup.sh" ] && . "$MODPATH/common_setup.sh"
[ -f "$MODPATH/sync_patch.sh" ] && sh "$MODPATH/sync_patch.sh" boot 2>/dev/null

# --- DenyList: intentionally NOT touched ---------------------------------
# With Zygisk Next / ReZygisk / NeoZygisk + a hider, enforcement stays OFF.
# We never flip the user's DenyList state.

# --- Bootloader / verified-boot props ------------------------------------
# Needed for DEVICE / STRONG verdict on custom ROMs and several OEMs.
# resetprop_if_diff only overrides a prop that already exists and differs,
# so it is harmless on devices where the value is already correct.

# Samsung
resetprop_if_diff ro.boot.warranty_bit 0
resetprop_if_diff ro.vendor.boot.warranty_bit 0
resetprop_if_diff ro.vendor.warranty_bit 0
resetprop_if_diff ro.warranty_bit 0
resetprop_if_diff ro.boot.fmp_config 1
resetprop_if_diff ro.boot.dp_fw_check 1

# Realme
resetprop_if_diff ro.boot.realmebootstate green

# OnePlus
resetprop_if_diff ro.is_ever_orange 0

# Encryption state
resetprop_if_diff ro.crypto.state encrypted

# VBMeta integrity chain (helps reach DEVICE on some devices)
resetprop_if_diff ro.boot.vbmeta.hash_alg sha256
resetprop_if_diff ro.boot.vbmeta.avb_version 1.0
resetprop_if_diff ro.boot.vbmeta.invalidate_on_error yes
for p in /dev/block/by-name/vbmeta /dev/block/by-name/vbmeta_a /dev/block/bootdevice/by-name/vbmeta; do
    [ -e "$p" ] && VBMETA_BLK="$p" && break
done
if [ -n "$VBMETA_BLK" ]; then
    VBMETA_SIZE=$(blockdev --getsize64 "$VBMETA_BLK" 2>/dev/null)
    [ -n "$VBMETA_SIZE" ] && resetprop_if_diff ro.boot.vbmeta.size "$VBMETA_SIZE"
fi

# Build tags / type — all variants (system, vendor, product, system_ext, ...)
for PROP in $(resetprop | grep -oE 'ro.*.build.tags'); do
    resetprop_if_diff "$PROP" release-keys
done
for PROP in $(resetprop | grep -oE 'ro.*.build.type'); do
    resetprop_if_diff "$PROP" user
done

resetprop_if_diff ro.adb.secure 1
if ! $SKIPDELPROP; then
    delprop_if_exist ro.boot.verifiedbooterror
    delprop_if_exist ro.boot.verifyerrorpart
fi
resetprop_if_diff ro.boot.veritymode.managed yes
resetprop_if_diff ro.debuggable 0
resetprop_if_diff ro.force.debuggable 0
resetprop_if_diff ro.secure 1

# --- Strip custom-ROM build leaks (LineageOS, crDroid, etc.) -------------
# These props are a dead giveaway to Play Integrity.
for PROP in ro.lineage.build.version ro.lineage.version ro.lineage.display.version \
            ro.modversion ro.cm.version ro.crdroid.version ro.crdroid.build.version \
            ro.evolution.version ro.pa.version ro.aosp.version; do
    delprop_if_exist "$PROP" 2>/dev/null || true
done

# --- Disable conflicting ROM-level spoof engines ------------------------
# (PixelPropsUtils / pihooks / entryhooks) before GMS starts.
# Opt-out: touch /data/adb/modules/integrityfateh7/no_rom_spoof_block
if [ -f "$MODPATH/rom_spoof_block.sh" ]; then
    sh "$MODPATH/rom_spoof_block.sh" 2>/dev/null || true
fi
