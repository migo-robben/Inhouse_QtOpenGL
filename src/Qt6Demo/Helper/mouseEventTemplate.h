void HelloWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && event->modifiers() == Qt::AltModifier) {
        // Alt + Left
        mousePos = event->pos();
        QMouseEvent fakeEvent(QEvent::MouseMove,
                              event->position(),
                              event->globalPosition(),
                              Qt::LeftButton,
                              event->buttons() | Qt::LeftButton,
                              Qt::AltModifier);
        mouseMoveEvent(&fakeEvent);
    }
    else if (event->button() == Qt::RightButton && event->modifiers() == Qt::AltModifier) {
        // Alt + Right
        mousePos = event->pos();

        zoomInProcessing = true;

        QMouseEvent fakeEvent(QEvent::MouseMove,
                              event->position(),
                              event->globalPosition(),
                              Qt::RightButton,
                              event->buttons() | Qt::RightButton,
                              Qt::AltModifier);
        mouseMoveEvent(&fakeEvent);
    }
    else if (event->buttons() == Qt::MiddleButton && event->modifiers() == Qt::AltModifier) {
        // Alt + Middle
        mousePos = event->pos();
        QMouseEvent fakeEvent(QEvent::MouseMove,
                              event->position(),
                              event->globalPosition(),
                              Qt::MiddleButton,
                              event->buttons() | Qt::MiddleButton,
                              Qt::AltModifier);
        mouseMoveEvent(&fakeEvent);
    }
    QWindow::mousePressEvent(event);
}

void HelloWindow::mouseReleaseEvent(QMouseEvent *event) {
    if (zoomInProcessing) {
        zoomInProcessing = false;
    }
    QWindow::mouseReleaseEvent(event);
}

void HelloWindow::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() == Qt::LeftButton && event->modifiers() == Qt::AltModifier) {
        QPoint offset = event->pos() - mousePos;
        camera->cameraRotateEvent(offset);
        // update viewport
        update();
        mousePos = event->pos();
    }
    else if (event->buttons() == Qt::RightButton && event->modifiers() == Qt::AltModifier) {
        QPoint offset = event->pos() - mousePos;

        camera->cameraZoomEvent(offset);

        // update viewport
        update();
        mousePos = event->pos();
    }
    else if (event->buttons() == Qt::MiddleButton && event->modifiers() == Qt::AltModifier) {
        QPoint offset = event->pos() - mousePos;

        camera->cameraTranslateEvent(offset);

        // update viewport
        update();
        mousePos = event->pos();
    }
    else {
        QWindow::mouseMoveEvent(event);
    }
}