# IRremoteESP8266 (LGPL-2.1, upstream tag v2.9.0) is kept as the *reference*
# source we port from. The library is being progressively re-implemented in
# C and owned under main/smart_flipper/lib/infrared/codec_db/. We do NOT
# build the upstream C++ here -- registering an empty component preserves
# the tree on disk for grep/diff while staying out of the link.
idf_component_register()
