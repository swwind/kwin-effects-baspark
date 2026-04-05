#pragma once

#include "effect/effect.h"
#include "effect/effecthandler.h"
#include "effect/timeline.h"

#include <QColor>
#include <QList>
#include <QPointF>
#include <QVector2D>
#include <QElapsedTimer>
#include <random>
#include <memory>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace KWin
{

struct TrailPoint {
    QPointF pos;
    float life = 1.0f;
};

struct Spark {
    float x, y, vx, vy, rot, rs, s, f;
    float a = 1.0f;
};

struct RingSeg {
    float off, len;
};

struct Wave {
    float x, y, r;
    float life = 0;
    float maxLife = 18.0f;
    struct {
        float ang, rs;
        float life = 0;
        float maxLife = 30.0f;
        QList<RingSeg> segs;
    } ring;
};

class BasparkEffect : public Effect
{
    Q_OBJECT

public:
    BasparkEffect();
    ~BasparkEffect() override;

    void prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime) override;
    void paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const Region &deviceRegion, LogicalOutput *screen) override;
    void postPaintScreen() override;
    void reconfigure(ReconfigureFlags flags) override;

    bool isActive() const override;

public Q_SLOTS:
    void slotMouseChanged(const QPointF &pos, const QPointF &oldPos,
                          Qt::MouseButtons buttons, Qt::MouseButtons oldButtons,
                          Qt::KeyboardModifiers modifiers, Qt::KeyboardModifiers oldModifiers);

private:
    void updateAnimation(float frames);
    void createBoom(float x, float y);
    void repaint();
    
    // Primitive Drawing
    void drawTrail(const RenderViewport &viewport);
    void drawWave(const RenderViewport &viewport, const Wave &w);
    void drawSpark(const RenderViewport &viewport, const Spark &s);
    
    void setupGL(const RenderTarget &renderTarget, const QMatrix4x4 &projectionMatrix);
    void finalizeGL();

    // Members
    QList<TrailPoint> m_trail;
    QList<Spark> m_sparks;
    QList<Wave> m_waves;

    QColor m_color = QColor(45, 175, 255);
    float m_scale = 1.5f;
    int m_maxTrailCount = 16;
    bool m_isMouseDown = false;
    QPointF m_lastMousePos;
    
    QElapsedTimer m_frameTimer;
    std::chrono::milliseconds m_lastPresentTime{0};
    std::mt19937 m_rng;
};

} // namespace KWin
