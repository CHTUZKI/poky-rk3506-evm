#!/usr/bin/env python3
"""Simple PyQt5 test to isolate the crash"""

import sys
import os

print("="*60)
print("PyQt5 Diagnostic Test")
print("="*60)

print("\nStep 1: Testing Python...")
print(f"Python version: {sys.version}")
print(f"Python executable: {sys.executable}")
print(f"sizeof(int): {sys.getsizeof(0)}")

print("\nStep 2: Checking Python modules path...")
print(f"sys.path: {sys.path}")

print("\nStep 3: Testing PyQt5 package...")
try:
    import PyQt5
    print(f"PyQt5 package found at: {PyQt5.__file__}")
except Exception as e:
    print(f"ERROR: Cannot find PyQt5 package: {e}")
    sys.exit(1)

print("\nStep 4: Testing PyQt5.QtCore...")
sys.stdout.flush()
try:
    from PyQt5 import QtCore
    print(f"✓ QtCore imported successfully")
    print(f"  Qt version: {QtCore.QT_VERSION_STR}")
    print(f"  PyQt version: {QtCore.PYQT_VERSION_STR}")
except Exception as e:
    print(f"✗ ERROR importing QtCore: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)

print("\nStep 5: Testing PyQt5.QtGui...")
sys.stdout.flush()
try:
    from PyQt5 import QtGui
    print(f"✓ QtGui imported successfully")
except Exception as e:
    print(f"✗ ERROR importing QtGui: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)

print("\nStep 6: Testing PyQt5.QtWidgets...")
sys.stdout.flush()
try:
    from PyQt5 import QtWidgets
    print(f"✓ QtWidgets imported successfully")
except Exception as e:
    print(f"✗ ERROR importing QtWidgets: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)

print("\n" + "="*60)
print("✓ All import tests passed!")
print("="*60)
print("\nPyQt5 modules are working correctly.")
print("To test GUI, run 'pyqt-test' with proper QT_QPA_PLATFORM setting.")
print("Example: QT_QPA_PLATFORM=eglfs pyqt-test")

