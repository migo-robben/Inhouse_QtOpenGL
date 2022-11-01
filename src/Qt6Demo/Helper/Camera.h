#ifndef INHOUSE_QTOPENGL_CAMERA_H
#define INHOUSE_QTOPENGL_CAMERA_H

#include <QVector3D>
#include <QMatrix4x4>

class Camera {
public:
    explicit Camera(
            QVector3D pos,
            QVector3D up = QVector3D(0.0, 1.0, 0.0),
            float yaw = 0.0f,
            float pitch = 0.0f,
            QVector3D initAimAt = QVector3D(0.0, 0.0, 0.0));

public:
    void updateCameraVectors();
    void setCameraPerspective(qreal aspect);
    QMatrix4x4 getCameraView();

    void rollBackToInitializeStatus();
    void rollBackRotate(QMatrix4x4 &matrix);
    void rollBackTranslate(QMatrix4x4 &matrix);
    void rollBackZoom(QMatrix4x4 &matrix);

    void cameraRotateEvent(QPoint offset);
    void cameraZoomEvent(QPoint offset);
    void cameraTranslateEvent(QPoint offset);

    void updateLimitZoom();

    float getDistanceFactor();

public:
    QMatrix4x4 projection;
    QMatrix4x4 view;
    qreal cameraAspect;

    QVector3D Front;
    QVector3D Right;
    QVector3D Up;
    QVector3D WorldUp;

    QVector3D defaultPosition;
    QVector3D defaultAimAt;

    qreal nearClipPlane;
    qreal farClipPlane;
    qreal fov;

    QVector3D aimAt;
    QVector3D position;

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

    QVector3D LimitZoom;
};


#endif //INHOUSE_QTOPENGL_CAMERA_H
