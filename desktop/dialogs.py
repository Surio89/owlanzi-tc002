# SPDX-License-Identifier: GPL-3.0-or-later
"""Readable application dialogs, independent of the operating system theme."""
from PySide6.QtCore import Qt
from PySide6.QtGui import QColor,QPalette
from PySide6.QtWidgets import QMessageBox

def light_palette():
    palette=QPalette()
    for role,color in ((QPalette.Window,'#f5f8f6'),(QPalette.WindowText,'#253431'),
                       (QPalette.Base,'#ffffff'),(QPalette.AlternateBase,'#e7f1eb'),
                       (QPalette.Text,'#253431'),(QPalette.Button,'#ffffff'),
                       (QPalette.ButtonText,'#253431'),(QPalette.Highlight,'#087f70'),
                       (QPalette.HighlightedText,'#ffffff')):
        palette.setColor(role,QColor(color))
    return palette

def message_box(parent,title,text,icon=QMessageBox.Information):
    box=QMessageBox(parent);box.setOption(QMessageBox.Option.DontUseNativeDialog,True)
    box.setWindowTitle(title);box.setTextFormat(Qt.PlainText);box.setText(text);box.setIcon(icon)
    box.setPalette(light_palette())
    box.setStyleSheet('QMessageBox{background:#f5f8f6} QLabel{color:#253431;background:transparent;font-size:14px} QPushButton{color:#253431;background:#fff;border:1px solid #91aaa0;border-radius:6px;padding:10px 16px;min-width:80px} QPushButton#confirm{color:#fff;background:#087f70;border:1px solid #087f70;font-weight:600} QPushButton:focus{border:2px solid #253431}')
    return box

def confirmation_box(parent,title,text,accept,cancel):
    box=message_box(parent,title,text,QMessageBox.Question)
    box.setStandardButtons(QMessageBox.Ok|QMessageBox.Cancel)
    box.button(QMessageBox.Ok).setText(accept);box.button(QMessageBox.Ok).setObjectName('confirm')
    box.button(QMessageBox.Cancel).setText(cancel)
    box.setDefaultButton(QMessageBox.Cancel);box.setEscapeButton(QMessageBox.Cancel)
    box.setStyleSheet(box.styleSheet())
    return box

def confirm(parent,title,text,accept,cancel):
    box=confirmation_box(parent,title,text,accept,cancel)
    try:return box.exec()==QMessageBox.Ok
    finally:box.deleteLater()

def inform(parent,title,text,icon=QMessageBox.Information):
    box=message_box(parent,title,text,icon);box.setStandardButtons(QMessageBox.Ok)
    try:box.exec()
    finally:box.deleteLater()
