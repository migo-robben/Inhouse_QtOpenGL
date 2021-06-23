#ifndef INHOUSE_QTOPENGL_CAMERA_H
#define INHOUSE_QTOPENGL_CAMERA_H


#include <QOpenGLShaderProgram>

class Camera {
private:
    QVector3D Front;
    QVector3D Right;
    QVector3D Up;
    QVector3D WorldUp;
    QVector3D initPosition;

    float Yaw;
    float Pitch;

    float MouseSensitivity = 0.30f;
    float MouseZoomSensitivity = 0.015f;
    float MouseBiasSensitivity = 0.008f;

    float PitchAngleLimit = 85.0f;

    float PitchOffsetAngle = 0.0f;
    float zoomFactor = 0.0f;

    float zoomLimit = 0.25f;

    float verticalBiasFactor = 0.0f;
    float horizontalBiasFactor = 0.0f;
    QVector3D defaultAimAt;

public:
    float MouseWheelSensitivity = 0.1f;
    float fovUpperBound = 105.0f;
    float fovLowerBound = 15.0f;

public:
    explicit Camera(
            QVector3D pos,
            QVector3D up = QVector3D(0.0, 1.0, 0.0),
            float yaw = 0.0f,
            float pitch = 0.0f,
            QVector3D initAimAt = QVector3D(0.0, 0.0, 0.0));
    ~Camera() = default;

    void setCameraPosition(QVector3D value) { position = value; }
    QVector3D getCameraPosition() const { return position; }

    void setCameraNearClipPlane(qreal value) { nearClipPlane = value; }
    qreal getCameraNearClipPlane() const { return nearClipPlane; }

    void setCameraFarClipPlane(qreal value) { farClipPlane = value; }
    qreal getCameraFarClipPlane() const { return farClipPlane; }

    void setCameraAimAt(QVector3D value) { aimAt = value; }
    QVector3D getCameraAimAt() { return aimAt; }

    void setCameraFov(qreal value) { fov = value; }
    qreal getCameraFov() const { return fov; }

    void setScreenSize(QSize screen_size);
    QSize getScreenSize();

    void setCameraPerspective(qreal aspect);
    QMatrix4x4 getCameraProjection();
    void setCameraPerspective(qreal aspect, int width, int height);

    QMatrix4x4 getOrthoCamera();

    float getDistanceFactor();

    QMatrix4x4 getCameraView();
    qreal getCameraAspect();

public:
    void rollBackToInitializeStatus();
    void rollBackRotate(QMatrix4x4 &matrix);
    void rollBackTranslate(QMatrix4x4 &matrix);
    void rollBackZoom(QMatrix4x4 &matrix);

    void cameraRotateEvent(QPoint offset);
    void cameraZoomEvent(QPoint offset);
    void cameraTranslateEvent(QPoint offset);

    void updateLimitZoom();

private:
    QMatrix4x4 view{};
    QMatrix4x4 projection{};
    QMatrix4x4 projectionOrtho{};

    qreal nearClipPlane{};
    qreal farClipPlane{};
    qreal fov{};

    QVector3D aimAt{};
    QVector3D position;

    QVector3D LimitZoom;

    qreal cameraAspect;
    QSize screenSize;
public:
    void updateCameraVectors();

};


#endif
