#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PyQt5 Simple Display Test for RK3506 EVM
简单的显示测试程序 - 480x800 DSI显示屏
"""

import sys
import os
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
                             QPushButton, QLabel, QLineEdit, QCheckBox, QRadioButton,
                             QSlider, QProgressBar, QSpinBox, QComboBox, QGroupBox)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont

class PyQtTestWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.click_count = 0
        self.init_ui()
        
    def init_ui(self):
        self.setWindowTitle("RK3506 Qt Widget Test")
        
        # 设置窗口大小匹配显示屏 (480x800)
        self.setGeometry(0, 0, 480, 800)
        
        # 创建中央控件
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # 创建主布局
        main_layout = QVBoxLayout()
        central_widget.setLayout(main_layout)
        
        # 标题标签
        title_label = QLabel("RK3506 Widget Test")
        title_font = QFont()
        title_font.setPointSize(16)
        title_font.setBold(True)
        title_label.setFont(title_font)
        title_label.setAlignment(Qt.AlignCenter)
        main_layout.addWidget(title_label)
        
        # 状态标签
        self.status_label = QLabel("Status: Ready")
        self.status_label.setAlignment(Qt.AlignCenter)
        main_layout.addWidget(self.status_label)
        
        # 按钮组
        button_group = QGroupBox("Buttons")
        button_layout = QVBoxLayout()
        
        # 第一行按钮
        btn_row1 = QHBoxLayout()
        self.btn1 = QPushButton("Button 1")
        self.btn1.clicked.connect(lambda: self.on_button_click(1))
        btn_row1.addWidget(self.btn1)
        
        self.btn2 = QPushButton("Button 2")
        self.btn2.clicked.connect(lambda: self.on_button_click(2))
        btn_row1.addWidget(self.btn2)
        
        self.btn3 = QPushButton("Button 3")
        self.btn3.clicked.connect(lambda: self.on_button_click(3))
        btn_row1.addWidget(self.btn3)
        button_layout.addLayout(btn_row1)
        
        # 第二行按钮
        btn_row2 = QHBoxLayout()
        self.btn4 = QPushButton("Button 4")
        self.btn4.clicked.connect(lambda: self.on_button_click(4))
        btn_row2.addWidget(self.btn4)
        
        self.btn5 = QPushButton("Button 5")
        self.btn5.clicked.connect(lambda: self.on_button_click(5))
        btn_row2.addWidget(self.btn5)
        
        self.btn6 = QPushButton("Button 6")
        self.btn6.clicked.connect(lambda: self.on_button_click(6))
        btn_row2.addWidget(self.btn6)
        button_layout.addLayout(btn_row2)
        
        button_group.setLayout(button_layout)
        main_layout.addWidget(button_group)
        
        # 文本输入框
        input_group = QGroupBox("Text Input")
        input_layout = QVBoxLayout()
        
        self.line_edit = QLineEdit()
        self.line_edit.setPlaceholderText("Enter text here...")
        self.line_edit.textChanged.connect(self.on_text_changed)
        input_layout.addWidget(self.line_edit)
        
        input_group.setLayout(input_layout)
        main_layout.addWidget(input_group)
        
        # 复选框和单选按钮
        check_radio_group = QGroupBox("CheckBox & RadioButton")
        check_radio_layout = QVBoxLayout()
        
        self.checkbox1 = QCheckBox("Option 1")
        self.checkbox1.stateChanged.connect(self.on_checkbox_changed)
        check_radio_layout.addWidget(self.checkbox1)
        
        self.checkbox2 = QCheckBox("Option 2")
        self.checkbox2.stateChanged.connect(self.on_checkbox_changed)
        check_radio_layout.addWidget(self.checkbox2)
        
        self.radio1 = QRadioButton("Radio A")
        self.radio1.toggled.connect(self.on_radio_changed)
        check_radio_layout.addWidget(self.radio1)
        
        self.radio2 = QRadioButton("Radio B")
        self.radio2.toggled.connect(self.on_radio_changed)
        check_radio_layout.addWidget(self.radio2)
        
        check_radio_group.setLayout(check_radio_layout)
        main_layout.addWidget(check_radio_group)
        
        # 滑动条和进度条
        slider_group = QGroupBox("Slider & Progress")
        slider_layout = QVBoxLayout()
        
        self.slider = QSlider(Qt.Horizontal)
        self.slider.setMinimum(0)
        self.slider.setMaximum(100)
        self.slider.setValue(50)
        self.slider.valueChanged.connect(self.on_slider_changed)
        slider_layout.addWidget(self.slider)
        
        self.progress_bar = QProgressBar()
        self.progress_bar.setValue(50)
        slider_layout.addWidget(self.progress_bar)
        
        slider_group.setLayout(slider_layout)
        main_layout.addWidget(slider_group)
        
        # 数字调节框和下拉框
        other_group = QGroupBox("SpinBox & ComboBox")
        other_layout = QHBoxLayout()
        
        self.spinbox = QSpinBox()
        self.spinbox.setMinimum(0)
        self.spinbox.setMaximum(100)
        self.spinbox.setValue(10)
        self.spinbox.valueChanged.connect(self.on_spinbox_changed)
        other_layout.addWidget(QLabel("Value:"))
        other_layout.addWidget(self.spinbox)
        
        self.combobox = QComboBox()
        self.combobox.addItems(["Item 1", "Item 2", "Item 3", "Item 4"])
        self.combobox.currentTextChanged.connect(self.on_combo_changed)
        other_layout.addWidget(QLabel("Select:"))
        other_layout.addWidget(self.combobox)
        
        other_group.setLayout(other_layout)
        main_layout.addWidget(other_group)
        
        # 底部计数器
        counter_layout = QHBoxLayout()
        counter_layout.addWidget(QLabel("Click Counter:"))
        self.counter_label = QLabel("0")
        counter_layout.addWidget(self.counter_label)
        counter_layout.addStretch()
        main_layout.addLayout(counter_layout)
        
        # 添加弹性空间
        main_layout.addStretch()
    
    def on_button_click(self, button_num):
        self.click_count += 1
        self.counter_label.setText(str(self.click_count))
        self.status_label.setText(f"Status: Button {button_num} clicked")
    
    def on_text_changed(self, text):
        self.status_label.setText(f"Status: Text = '{text}'")
    
    def on_checkbox_changed(self, state):
        sender = self.sender()
        status = "checked" if state == Qt.Checked else "unchecked"
        self.status_label.setText(f"Status: {sender.text()} {status}")
    
    def on_radio_changed(self, checked):
        if checked:
            sender = self.sender()
            self.status_label.setText(f"Status: {sender.text()} selected")
    
    def on_slider_changed(self, value):
        self.progress_bar.setValue(value)
        self.status_label.setText(f"Status: Slider = {value}")
    
    def on_spinbox_changed(self, value):
        self.status_label.setText(f"Status: SpinBox = {value}")
    
    def on_combo_changed(self, text):
        self.status_label.setText(f"Status: Selected '{text}'")
    
    def keyPressEvent(self, event):
        """处理键盘事件"""
        if event.key() == Qt.Key_Escape:
            self.close()
        super().keyPressEvent(event)

def main():
    # 设置QPA平台为嵌入式设备
    # 优先使用eglfs (GPU加速)，备选linuxfb
    if 'QT_QPA_PLATFORM' not in os.environ:
        # 检查eglfs是否可用
        if os.path.exists('/usr/lib/plugins/platforms/libqeglfs.so'):
            os.environ['QT_QPA_PLATFORM'] = 'eglfs'
            print("Using QPA platform: eglfs")
        elif os.path.exists('/usr/lib/plugins/platforms/libqlinuxfb.so'):
            os.environ['QT_QPA_PLATFORM'] = 'linuxfb'
            print("Using QPA platform: linuxfb")
        else:
            # 降级到offscreen用于测试
            os.environ['QT_QPA_PLATFORM'] = 'offscreen'
            print("Using QPA platform: offscreen (no display)")
    
    app = QApplication(sys.argv)
    
    # 使用传统的Windows样式（可选：'Windows', 'Fusion', 'Plastique'等）
    # 注释掉可使用默认样式
    # app.setStyle('Windows')
    
    # 创建并显示窗口
    window = PyQtTestWindow()
    window.show()
    
    # 如果需要全屏，取消注释下面这行
    # window.showFullScreen()
    
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()

