#include "inputHandler.h"

#include "glm/glm.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "platform.h"
#include <cmath>

namespace Tangram {

InputHandler::InputHandler(std::shared_ptr<View> _view) : m_view(_view) {}

void InputHandler::update(float _dt) {

    auto screenVelocityTranslate = m_view->pixelsPerMeter() * m_deltaTranslate;

    bool isFlinging = glm::length(screenVelocityTranslate) > m_thresholdStopTranslate || std::abs(m_deltaZoom) > m_thresholdStopZoom;

    if (!m_gestureOccured && isFlinging) {

        m_deltaTranslate -= _dt * m_dampingTranslate * m_deltaTranslate;
        m_view->translate(_dt * m_deltaTranslate.x, _dt * m_deltaTranslate.y);

        m_deltaZoom -= _dt * m_dampingZoom * m_deltaZoom;
        m_view->zoom(m_deltaZoom * _dt);

        requestRender();

    }

    m_gestureOccured = false;
}

void InputHandler::handleTapGesture(float _posX, float _posY) {

    onGesture();

    double viewCenterX = 0.5 * m_view->getWidth();
    double viewCenterY = 0.5 * m_view->getHeight();

    m_view->screenToGroundPlane(viewCenterX, viewCenterY);
    m_view->screenToGroundPlane(_posX, _posY);

    m_view->translate((_posX - viewCenterX), (_posY - viewCenterY));

}

void InputHandler::handleDoubleTapGesture(float _posX, float _posY) {

    handlePinchGesture(_posX, _posY, 2.f, 0.f);

}

void InputHandler::handlePanGesture(float _startX, float _startY, float _endX, float _endY) {

    onGesture();

    m_view->screenToGroundPlane(_startX, _startY);
    m_view->screenToGroundPlane(_endX, _endY);

    float dx = _startX - _endX;
    float dy = _startY - _endY;

    m_view->translate(dx, dy);

}

void InputHandler::handleFlingGesture(float _posX, float _posY, float _velocityX, float _velocityY) {

    if (glm::length(glm::vec2(_velocityX, _velocityY)) <= m_thresholdStartTranslate) {
        return;
    }

    const static float epsilon = 0.0167f;

    float startX = _posX;
    float startY = _posY;
    float endX = _posX + epsilon * _velocityX;
    float endY = _posY + epsilon * _velocityY;

    m_view->screenToGroundPlane(startX, startY);
    m_view->screenToGroundPlane(endX, endY);

    float dx = (startX - endX) / epsilon;
    float dy = (startY - endY) / epsilon;

    setDeltas(0.f, glm::vec2(dx, dy));

}

void InputHandler::handlePinchGesture(float _posX, float _posY, float _scale, float _velocity) {

    onGesture();

    float z = m_view->getZoom();
    static float invLog2 = 1 / log(2);
    m_view->zoom(log(_scale) * invLog2);

    m_view->screenToGroundPlane(_posX, _posY);
    float s = pow(2, m_view->getZoom() - z) - 1;
    m_view->translate(s * _posX, s * _posY);

    // Take the derivative of zoom as a function of scale:
    // z(s) = log2(s) + C
    // z'(s) = s' / s / log(2)
    float zoomVelocity = _velocity / _scale * invLog2;
    if (std::abs(zoomVelocity) >= m_thresholdStartZoom) {
        setDeltas(zoomVelocity, glm::vec2(0.f));
    }

}

void InputHandler::handleRotateGesture(float _posX, float _posY, float _radians) {

    onGesture();
    m_view->roll(_radians);

}

void InputHandler::handleShoveGesture(float _distance) {

    onGesture();

    float angle = -M_PI * _distance / m_view->getHeight();
    m_view->pitch(angle);

}

void InputHandler::onGesture() {

    m_gestureOccured = true;
    setDeltas(0.f, { 0.f, 0.f });
    requestRender();

}

void InputHandler::setDeltas(float _zoom, glm::vec2 _translate) {

    // setup deltas for momentum on gesture
    m_deltaTranslate = _translate;
    m_deltaZoom = _zoom;
}

}
