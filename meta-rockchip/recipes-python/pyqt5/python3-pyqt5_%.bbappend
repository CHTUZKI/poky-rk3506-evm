# Copyright (c) 2025, Rockchip Electronics Co., Ltd
# Released under the MIT license (see COPYING.MIT for the terms)
#
# Fix Python header path issue in PyQt5 cross-compilation
# Problem: sip-build uses recipe-sysroot-native Python headers (x86_64) instead of target headers (ARM)
# Solution: Replace all Python paths in .pro files to use recipe-sysroot (target) instead of recipe-sysroot-native
#

# Fix all .pro files to use target Python headers and libraries instead of native
# This must be done after sip-build generates the .pro files
do_configure:append() {
    bbnote "Fixing Python paths in all .pro files to use target sysroot"
    
    # Find all .pro files and fix Python paths (both INCLUDEPATH and LIBS)
    find "${B}" -name "*.pro" -type f | while read pro_file; do
        if grep -q "recipe-sysroot-native.*python" "$pro_file"; then
            # Replace native Python include paths with target paths
            sed -i 's|recipe-sysroot-native/usr/include/python|recipe-sysroot/usr/include/python|g' "$pro_file"
            
            # Replace native Python library paths with target paths
            # Pattern 1: -L/path/to/recipe-sysroot-native/usr/lib
            sed -i 's|recipe-sysroot-native/usr/lib[^[:space:]]*|recipe-sysroot/usr/lib|g' "$pro_file"
            
            # Pattern 2: LIBS += -L/path/to/recipe-sysroot-native/usr/lib
            sed -i 's|-L[^[:space:]]*recipe-sysroot-native/usr/lib|-L\$\$[QT_SYSROOT]/usr/lib|g' "$pro_file"
            
            bbnote "Fixed Python paths in $(basename $pro_file)"
        fi
    done
    
    # Regenerate all Makefiles with fixed .pro files
    bbnote "Regenerating Makefiles with corrected Python paths"
    for module_dir in "${B}"/Qt* "${B}"/qmlscene "${B}"/pylupdate "${B}"/pyrcc "${B}"/Qt; do
        if [ -d "$module_dir" ]; then
            pro_file="$module_dir/$(basename $module_dir).pro"
            if [ -f "$pro_file" ]; then
                cd "$module_dir" && ${OE_QMAKE_QMAKE} "$(basename $pro_file)"
                bbnote "Regenerated Makefile for $(basename $module_dir)"
            fi
        fi
    done
    
    # Also fix Makefiles directly in case qmake doesn't regenerate them properly
    find "${B}" -name "Makefile" -type f | while read makefile; do
        if grep -q "recipe-sysroot-native/usr/lib.*python" "$makefile"; then
            sed -i 's|recipe-sysroot-native/usr/lib|recipe-sysroot/usr/lib|g' "$makefile"
            bbnote "Fixed Python library paths in $(basename $(dirname $makefile))/Makefile"
        fi
    done
}

# Ensure all modules are compiled
do_compile:append() {
    cd ${B}
    oe_runmake
}

# Install all PyQt5 modules
do_install:prepend() {
    # Fix qmlscene Makefile install paths if needed
    # qmake may expand paths to absolute paths including recipe-sysroot
    qmlscene_makefile="${B}/qmlscene/Makefile"
    if [ -f "$qmlscene_makefile" ]; then
        if grep -q "recipe-sysroot/usr/lib/plugins" "$qmlscene_makefile"; then
            bbnote "Fixing qmlscene Makefile install paths"
            sed -i 's|/[^[:space:]]*recipe-sysroot/usr/lib/plugins/PyQt5|/usr/lib/plugins/PyQt5|g' "$qmlscene_makefile"
        fi
    fi
    
    # Run make install to install all Python modules
    bbnote "Installing all PyQt5 modules"
    cd ${B} && make install INSTALL_ROOT=${D} || bbfatal "make install failed"
    
    # Disable errexit for original recipe's sed command
    set +e
}

# Clean up after original recipe's do_install:append
do_install:append() {
    # Re-enable errexit
    set -e
}
