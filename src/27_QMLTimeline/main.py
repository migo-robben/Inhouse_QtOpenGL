import sys, os

from PySide2 import QtQml, QtCore, QtGui

class processHelper(QtCore.QObject):
    def __init__(self):
        QtCore.QObject.__init__(self)


    @QtCore.Slot()
    def simpleTest(self):
        print("Just Simple Test")


if __name__ == "__main__":
    app = QtGui.QGuiApplication(sys.argv)
    engine = QtQml.QQmlApplicationEngine()

    helper = processHelper()
    engine.rootContext().setContextProperty("backend", helper)

    # Load QML File
    engine.load(os.path.join(os.path.dirname(__file__), "QML/basicWindow.qml"))

    if not engine.rootObjects():
        sys.exit(-1)
    sys.exit(app.exec_())
