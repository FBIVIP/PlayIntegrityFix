integrityfateh7 — TEESim two-path split patch
==============================================
Split:
  KEYBOX_PATH  = /data/misc/the_next   (keybox.xml only)
  CONFIG_PATH  = /data/adb/fateh7       (target.txt, security_patch.txt,
                                          hbk, persistent_keys, logs, boot_*)

HOW TO APPLY (in your PlayIntegrityFix / TEESim Codespace):
  1. Copy the 4 .kt files below over the originals (same paths):
       app/src/main/java/org/matrix/TEESimulator/config/ConfigurationManager.kt
       app/src/main/java/org/matrix/TEESimulator/pki/KeyBoxManager.kt
       app/src/main/java/org/matrix/TEESimulator/util/AndroidDeviceUtils.kt
       app/src/main/java/org/matrix/TEESimulator/pki/NativeCertGen.kt
  2. Commit + push  ->  build TEESim (Actions)  ->  download the release zip
  3. Rebuild the module with:  bash build.sh --tee-file ../<new TEESim zip>
