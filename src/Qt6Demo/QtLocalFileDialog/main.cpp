#include <QtWidgets>

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #include <emscripten/html5.h>
    #include <emscripten/val.h>
    #include <emscripten/bind.h>

    using namespace emscripten;
    typedef void (*FileDataCallback)(void *context, char *data, size_t length, const char *name);

    void readFiles(val event) {
        val target = event["target"];
        val files = target["files"];

        EM_ASM(
            console.log("Read Files");
        );
    }

    EMSCRIPTEN_BINDINGS(localfileaccess) {
        function("qtReadFiles", &readFiles);
    }

    void loadFile(const char *accept, FileDataCallback callback, void *context)
    {
        // Create file input element which will dislay a native file dialog.
        val document = val::global("document");
        val input = document.call<val>("createElement", std::string("input"));
        input.set("type", "file");
        input.set("style", "display:none");
        input.set("accept", val(accept));

        input.set("onchange", val::module_property("qtReadFiles"));

        // Programatically activate input
        val body = document["body"];
        body.call<void>("appendChild", input);
        input.call<void>("click");
        body.call<void>("removeChild", input);
    }

    extern "C" EMSCRIPTEN_KEEPALIVE void testLoadFile()
    {
        void *context = nullptr;
        loadFile("All Files (*.*)", [](void *context, char *contentPointer, size_t contentSize, const char *fileName) {
            printf("File loaded %p %d %s\n", contentPointer, contentSize, fileName);
            printf("First bytes: %x %x %x %x\n", contentPointer[0] & 0xff, contentPointer[1] & 0xff,
                                                 contentPointer[2] & 0xff, contentPointer[3] & 0xff);
            free(contentPointer);
        }, context);
    }
#endif

class myFileDialog : public QWidget {
public:
    myFileDialog() {
        setFixedSize(300, 200);
        setWindowTitle("My FileDialog");
        setPalette(QPalette(QColor("#2F2F2F")));
        setAutoFillBackground(true);

        initUI();
    }

    ~myFileDialog() override {
        delete btn;
        delete label;
        delete customLayout;
    }

    void initUI() {
        customLayout = new QHBoxLayout();
        customLayout->setContentsMargins(10, 10, 10, 10);
        customLayout->setSpacing(0);

        setLayout(customLayout);

        btn = new QPushButton("Get File");
        btn->setFixedSize(this->height(), this->height()-20);
        btn->setFlat(true);
        connect(btn, &QPushButton::clicked, this, &myFileDialog::getFiles);

        label = new QLabel("Size: ");

        customLayout->addWidget(btn);
        customLayout->addWidget(label);
    }

    void getFiles() {
        QString fname = QFileDialog::getOpenFileName(this, tr("Open file"), "", tr("All Files (*.*)"));
        QImage img = QImage(fname);
        label->setText(
                "Size: " + QString::number(img.width()) + "*" + QString::number(img.height()));
        QFile inputFile(fname);
    }

protected:
    QHBoxLayout *customLayout;
    QPushButton *btn;
    QLabel *label;
};


int main(int argc, char **argv)
{
//#ifdef __EMSCRIPTEN__
//    EM_ASM(
//        // Make a directory other than '/'
//        FS.mkdir('/offline');
//        // Then mount with IDBFS type
//        FS.mount(MEMFS, {}, '/offline');
//        );
//#endif
    QApplication app(argc, argv);

    QImage img = QImage(QString(":/images/awesomeface.png"));
    printf("Image height: %d\n", img.height());

    myFileDialog window;
    window.show();

    return QApplication::exec();
}