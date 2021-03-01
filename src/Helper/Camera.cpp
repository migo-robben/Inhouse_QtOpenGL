#include "Camera.h"
#include <cmath>


Camera::Camera(QVector3D pos, QVector3D up, float yaw, float pitch, QVector3D initAimAt)
        : nearClipPlane(0.01),
          farClipPlane(10000.0),
          fov(45.0),
          aimAt(initAimAt),
          position(pos),
          WorldUp(up),
          Yaw(yaw),
          Pitch(pitch) {

    initPosition = position;
    defaultAimAt = aimAt;

    updateCameraVectors();
    updateLimitZoom();
}

void Camera::setCameraPerspective(qreal aspect) {
    projection.setToIdentity();
    projection.perspective(fov, aspect, nearClipPlane, farClipPlane);
}

QMatrix4x4 Camera::getCameraProjection() {
    return projection;
}

QMatrix4x4 Camera::getCameraView() {
    view.setToIdentity();
    view.lookAt(position, aimAt, WorldUp);
    return view;
}

void Camera::updateCameraVectors() {
    Front = (position - aimAt).normalized();
    Right = QVector3D::crossProduct(WorldUp, Front).normalized();
    Up = QVector3D::crossProduct(Front, Right).normalized();
}

void Camera::updateLimitZoom() {
    LimitZoom = (getCameraPosition() - getCameraAimAt()).normalized() - zoomLimit*Front;
}

float Camera::getDistanceFactor() {
    return 0.15f * std::pow((position - aimAt).length(), 1.1f);
}

void Camera::rollBackToInitializeStatus() {
    setCameraPosition(initPosition);
    setCameraAimAt(defaultAimAt);
    updateCameraVectors();
}

void Camera::rollBackRotate(QMatrix4x4 &matrix) {
    // Update Yaw and Pitch
    matrix.rotate(QQuaternion::fromAxisAndAngle(Up, Yaw));
    matrix.rotate(QQuaternion::fromAxisAndAngle(Right, Pitch));

    setCameraPosition(matrix * getCameraPosition());
    updateCameraVectors();
}

void Camera::rollBackTranslate(QMatrix4x4 &matrix) {
    // up-down, right-left translate
    matrix.setToIdentity();
    matrix.translate(Up * horizontalBiasFactor);
    matrix.translate(Right * verticalBiasFactor);

    setCameraPosition(matrix * getCameraPosition());
    setCameraAimAt(getCameraAimAt() + Right * verticalBiasFactor + Up * horizontalBiasFactor);
    updateCameraVectors();
}

void Camera::rollBackZoom(QMatrix4x4 &matrix) {
    // zoom translate
    matrix.setToIdentity();
    matrix.translate(Front * zoomFactor);
    setCameraPosition(matrix * getCameraPosition());
}

void Camera::cameraRotateEvent(QPoint offset) {
    rollBackToInitializeStatus();

    // Initialize transform matrix
    QMatrix4x4 matrix;
    matrix.setToIdentity();

    // Update Yaw
    Yaw += -1 * MouseSensitivity * (float)offset.x();
    matrix.rotate(QQuaternion::fromAxisAndAngle(Up, Yaw));

    // Update Pitch
    Pitch += -1 * MouseSensitivity * (float)offset.y();
    if ((Pitch + PitchOffsetAngle) < -PitchAngleLimit || (Pitch + PitchOffsetAngle) > PitchAngleLimit) {
        if ((Pitch + PitchOffsetAngle) < -PitchAngleLimit)
            Pitch = -PitchAngleLimit - PitchOffsetAngle;
        else
            Pitch = PitchAngleLimit - PitchOffsetAngle;
    }
    matrix.rotate(QQuaternion::fromAxisAndAngle(Right, Pitch));

    setCameraPosition(matrix * getCameraPosition());
    updateCameraVectors();

    // zoom translate
    rollBackZoom(matrix);

    // up-down, right-left translate
    rollBackTranslate(matrix);

    updateLimitZoom();
}

void Camera::cameraZoomEvent(QPoint offset) {
    rollBackToInitializeStatus();

    // Initialize transform matrix
    QMatrix4x4 matrix;
    matrix.setToIdentity();

    // Update Yaw and Pitch
    rollBackRotate(matrix);

    // zoom translate
    zoomFactor += (float)-offset.x() * MouseZoomSensitivity;
    float dotResult = QVector3D::dotProduct((getCameraAimAt() - (getCameraPosition() + Front * zoomFactor)).normalized(), (getCameraAimAt() - getCameraPosition()).normalized());
    if ((getCameraAimAt() - (getCameraPosition() + Front * zoomFactor)).length() < LimitZoom.length() || dotResult < 0) {
        zoomFactor = -(getCameraPosition() - LimitZoom).length();
    }
    matrix.setToIdentity();
    matrix.translate(Front * zoomFactor);
    setCameraPosition(matrix * getCameraPosition());

    // up-down, right-left translate
    rollBackTranslate(matrix);
}

void Camera::cameraTranslateEvent(QPoint offset) {
    rollBackToInitializeStatus();

    // Initialize transform matrix
    QMatrix4x4 matrix;
    matrix.setToIdentity();

    // Update Yaw and Pitch
    rollBackRotate(matrix);

    // zoom translate
    rollBackZoom(matrix);

    // up-down, right-left translate
    matrix.setToIdentity();
    verticalBiasFactor += (float)-offset.x() * MouseBiasSensitivity * getDistanceFactor();
    horizontalBiasFactor += (float)offset.y() * MouseBiasSensitivity * getDistanceFactor();
    matrix.translate(Up * horizontalBiasFactor);
    matrix.translate(Right * verticalBiasFactor);

    setCameraPosition(matrix * getCameraPosition());
    setCameraAimAt(getCameraAimAt() + Right * verticalBiasFactor + Up * horizontalBiasFactor);
    updateCameraVectors();

    updateLimitZoom();
}