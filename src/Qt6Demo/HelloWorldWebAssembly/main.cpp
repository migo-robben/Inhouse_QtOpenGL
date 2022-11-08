#include <QtWidgets>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

class myWidget : public QDialog {
Q_OBJECT
public:
    myWidget() {
        // set current dialog widget's size
        this->setFixedSize(640, 480);

        // initialize UI
        InitUI();
    }

public:
    void InitUI() {
        // create component
        hlayout = new QHBoxLayout(this);
        myBtn = new QPushButton("Click Me !");
        imageLabel = new QLabel();

        imageLabel->setPixmap(QPixmap(":/images/awesomeface.png"));
        connect(myBtn, &QPushButton::clicked, this, &myWidget::printInfo);

        // add widget to layout
        hlayout->addWidget(myBtn);
        hlayout->addWidget(imageLabel);

        // set current layout to widget
        this->setLayout(hlayout);
    }

protected:
    void printInfo() {
        qDebug() << "Click Me Button Clicked !";
#ifdef __EMSCRIPTEN__
        // glue to js console.log function
        EM_ASM( { console.log("Click Me Button Clicked[JS] !")}; );
#endif
    }

private:
    QHBoxLayout *hlayout;
    QPushButton *myBtn;
    QLabel *imageLabel;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    myWidget window;

#ifdef __EMSCRIPTEN__
    window.showFullScreen();
#else
    window.show();
#endif

    return QApplication::exec();
}

#include "main.moc"
