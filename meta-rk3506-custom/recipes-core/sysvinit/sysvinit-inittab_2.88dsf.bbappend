# Copyright (C) 2024 Rockchip Electronics Co., Ltd.
# Fix for busybox compatibility - avoid using tail

do_install() {
    install -d ${D}${sysconfdir}
    install -m 0644 ${WORKDIR}/inittab ${D}${sysconfdir}/inittab
    install -d ${D}${base_bindir}
    install -m 0755 ${WORKDIR}/start_getty ${D}${base_bindir}/start_getty
    sed -e 's,/usr/bin,${bindir},g' -i ${D}${base_bindir}/start_getty

    CONSOLES="${SERIAL_CONSOLES}"
    for s in $CONSOLES
    do
        speed=$(echo $s | cut -d';' -f 1)
        device=$(echo $s | cut -d';' -f 2)
        # Get last 4 chars of device name (e.g., ttyFIQ0 -> FIQ0, ttyS0 -> ttyS0)
        # Use awk instead of tail for busybox compatibility
        label=$(echo $device | sed -e 's/tty//' | awk '{print substr($0, length-3)}')
        # If label is empty or too short, use full device name
        if [ -z "$label" ]; then
            label="$device"
        fi

        echo "$label:12345:respawn:${sbindir}/ttyrun $device ${base_bindir}/start_getty $speed $device vt102" >> ${D}${sysconfdir}/inittab
    done

    if [ "${USE_VT}" = "1" ]; then
        cat <<EOF >>${D}${sysconfdir}/inittab
# ${base_sbindir}/getty invocations for the runlevels.
#
# The "id" field MUST be the same as the last
# characters of the device (after "tty").
#
# Format:
#  <id>:<runlevels>:<action>:<process>
#

EOF

        for n in ${SYSVINIT_ENABLED_GETTYS}
        do
            echo "$n:12345:respawn:${base_sbindir}/getty 38400 tty$n" >> ${D}${sysconfdir}/inittab
        done
        echo "" >> ${D}${sysconfdir}/inittab
    fi
}
