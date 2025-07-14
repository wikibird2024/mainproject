#!/bin/bash

COMP="sim4g_gps"
mkdir -p $COMP/{inc,src,include_private,examples}

cat > $COMP/CMakeLists.txt << EOF
idf_component_register(
    SRCS 
        "src/sim4g_gps.c"
        "src/sim_at.c"
        "src/sim_gps.c"
        "src/sim_net.c"
        "src/sim_util.c"
    INCLUDE_DIRS "inc"
    PRIV_INCLUDE_DIRS "include_private"
    PRIV_REQUIRES freertos esp_log driver comm debugs
)
EOF

cat > $COMP/Kconfig << EOF
menu "SIM4G_GPS Configuration"

config SIM4G_GPS_ENABLE
    bool "Enable SIM4G GPS module"
    default y

endmenu
EOF

cat > $COMP/inc/sim4g_gps.h << EOF
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_SIM4G_GPS_ENABLE
void sim4g_gps_init(void);
#endif

#ifdef __cplusplus
}
#endif
EOF

for f in sim4g_gps sim_at sim_gps sim_net sim_util; do
    echo "// $f.c" > $COMP/src/$f.c
done

for f in sim_at sim_gps sim_net sim_util; do
cat > $COMP/include_private/$f.h << EOF
#pragma once
// Internal API for $f.c
EOF
done

cat > $COMP/src/sim4g_gps.c << EOF
#include "sim4g_gps.h"
#include "debugs.h"
#include "sim_at.h"
#include "sim_gps.h"
#include "sim_net.h"
#include "sim_util.h"

#if CONFIG_SIM4G_GPS_ENABLE
void sim4g_gps_init(void) {
    debug_log("SIM4G_GPS initializing...");
}
#endif
EOF

cat > $COMP/examples/example_main.c << EOF
#include "sim4g_gps.h"

void app_main(void) {
#if CONFIG_SIM4G_GPS_ENABLE
    sim4g_gps_init();
#endif
}
EOF

echo "[✓] Component '$COMP' initialized."
