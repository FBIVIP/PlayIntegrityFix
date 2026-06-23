#!/system/bin/sh
MODDIR=${0%/*}
CONFIG_DIR=/data/misc/the_next

. "$MODDIR/action_i18n.sh"

echo "  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  ⚠️  $(_msg confirm_header)"
echo "  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo " "
echo "  $(_msg confirm_warning_1)"
echo "  $(_msg confirm_warning_2)"
echo " "
echo "  🔊  $(_msg confirm_vol_up)"
echo "  🔉  $(_msg confirm_vol_down)"
echo " "

confirm() {
    # Sample getevent in 1s bursts; a piped stream block-buffers and misses
    # a single key-press before the timeout.
    deadline=$(( $(date +%s) + 10 ))
    while [ "$(date +%s)" -lt "$deadline" ]; do
        events=$(/system/bin/timeout 1 /system/bin/getevent -l 2>/dev/null)
        case "$events" in
            *KEY_VOLUMEUP*)   return 0 ;;
            *KEY_VOLUMEDOWN*) return 1 ;;
        esac
    done
    return 1
}

if ! confirm; then
    echo " "
    echo "  ❌ $(_msg confirm_cancelled)"
    exit 0
fi

if [ -d "$CONFIG_DIR/persistent_keys" ]; then
    rm -rf "$CONFIG_DIR/persistent_keys"
    mkdir -p "$CONFIG_DIR/persistent_keys"
    echo " "
    echo "  ✅ $(_msg confirm_cleared)"
else
    echo " "
    echo "  ℹ️  $(_msg confirm_not_found)"
fi
