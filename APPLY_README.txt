integrityfateh7 — REVERT to UNIFIED /data/misc/the_next
========================================================
Undoes the two-path split. All 5 files back to ONE path:
  CONFIG_PATH = /data/misc/the_next  (keybox, target.txt, security_patch.txt,
                hbk, persistent_keys, logs, boot_*) — XOR-hidden, no KEYBOX_PATH.

APPLY (in PlayIntegrityFix Codespace):
  1. unzip -o teesim-unify-revert.zip   (overwrites 5 .kt files)
  2. git add -A && git commit -m "revert to unified the_next" && git push
  3. build via Actions, download the new (unified) TEESim zip
