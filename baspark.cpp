#include "baspark.h"

#include "core/rendertarget.h"
#include "core/renderviewport.h"
#include "effect/effecthandler.h"
#include "opengl/glshader.h"
#include "opengl/glshadermanager.h"
#include "opengl/glvertexbuffer.h"

#include <cmath>
#include <span>

namespace KWin
{

BasparkEffect::BasparkEffect()
{
    m_rng.seed(std::random_device{}());
    m_frameTimer.start();
    connect(effects, &EffectsHandler::mouseChanged, this, &BasparkEffect::slotMouseChanged);
}

BasparkEffect::~BasparkEffect()
{
    // Explicitly disconnect to prevent signals firing during destruction
    disconnect(effects, &EffectsHandler::mouseChanged, this, &BasparkEffect::slotMouseChanged);
}

void BasparkEffect::prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime)
{
    // Prevent double updates in multi-output setups
    if (m_lastPresentTime == presentTime) {
        effects->prePaintScreen(data, presentTime);
        return;
    }

    qint64 elapsed = m_frameTimer.restart();
    if (m_lastPresentTime.count() == 0) elapsed = 16;
    m_lastPresentTime = presentTime;

    float frames = std::min(elapsed / 16.666f, 3.0f);
    updateAnimation(frames);

    effects->prePaintScreen(data, presentTime);
}

void BasparkEffect::updateAnimation(float frames)
{
    // 1:1 Logic Implementation
    for (auto it = m_trail.begin(); it != m_trail.end(); ) {
        it->life -= (m_isMouseDown ? 0.085f : 0.18f) * frames;
        if (it->life <= 0) it = m_trail.erase(it); else ++it;
    }

    for (auto it = m_waves.begin(); it != m_waves.end(); ) {
        it->life += frames;
        float progress = std::min(it->life / it->maxLife, 1.0f);
        it->r = 26.0f * m_scale * (1.0f - std::pow(1.0f - progress, 3));
        
        it->ring.life += frames;
        it->ring.ang -= it->ring.rs * frames;

        if (it->life >= it->maxLife && it->ring.life >= it->ring.maxLife) {
            it = m_waves.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = m_sparks.begin(); it != m_sparks.end(); ) {
        it->x += it->vx * frames;
        it->y += it->vy * frames;
        it->vx *= std::pow(it->f, frames);
        it->vy *= std::pow(it->f, frames);
        it->rot += it->rs * frames;
        it->a -= 0.032f * frames;
        if (it->a <= 0) it = m_sparks.erase(it); else ++it;
    }
}

void BasparkEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const Region &deviceRegion, LogicalOutput *screen)
{
    effects->paintScreen(renderTarget, viewport, mask, deviceRegion, screen);

    if (!isActive() || !effects->isOpenGLCompositing()) {
        return;
    }

    setupGL(renderTarget, viewport.projectionMatrix());

    drawTrail(viewport);
    for (const auto &w : m_waves) drawWave(viewport, w);
    for (const auto &s : m_sparks) drawSpark(viewport, s);

    finalizeGL();
}

void BasparkEffect::postPaintScreen()
{
    effects->postPaintScreen();
    repaint();
}

void BasparkEffect::reconfigure(ReconfigureFlags)
{
}

bool BasparkEffect::isActive() const
{
    return !m_trail.isEmpty() || !m_waves.isEmpty() || !m_sparks.isEmpty();
}

void BasparkEffect::slotMouseChanged(const QPointF &pos, const QPointF &,
                                     Qt::MouseButtons buttons, Qt::MouseButtons oldButtons,
                                     Qt::KeyboardModifiers, Qt::KeyboardModifiers)
{
    bool wasDown = oldButtons & Qt::LeftButton;
    m_isMouseDown = buttons & Qt::LeftButton;

    if (m_isMouseDown && !wasDown) {
        m_lastMousePos = pos;
        createBoom(pos.x(), pos.y());
    }

    if (m_isMouseDown) {
        float d = std::hypot(pos.x() - m_lastMousePos.x(), pos.y() - m_lastMousePos.y());
        if (d > 2.0f) {
            m_trail.append({pos, 1.0f});
            if (m_trail.size() > m_maxTrailCount) m_trail.removeFirst();
            
            std::uniform_real_distribution<float> dis(0, 1);
            if (dis(m_rng) < 0.3f) {
                float a = dis(m_rng) * M_PI * 2.0f;
                m_sparks.append({
                    static_cast<float>(pos.x()) + std::cos(a) * 10.0f * m_scale,
                    static_cast<float>(pos.y()) + std::sin(a) * 10.0f * m_scale,
                    std::cos(a) * 1.3f, std::sin(a) * 1.3f,
                    dis(m_rng) * static_cast<float>(M_PI) * 2.0f, 0.16f, 9.0f * m_scale, 0.95f, 0.7f
                });
            }
            m_lastMousePos = pos;
        }
    }
    repaint();
}

void BasparkEffect::createBoom(float x, float y)
{
    std::uniform_real_distribution<float> dis(0, 1);
    Wave w;
    w.x = x; w.y = y;
    w.ring.ang = dis(m_rng) * static_cast<float>(M_PI) * 2.0f;
    w.ring.rs = 0.08f;
    w.ring.segs = {{-0.25f * static_cast<float>(M_PI), 1.15f * static_cast<float>(M_PI)},
                   { 0.00f * static_cast<float>(M_PI), 1.15f * static_cast<float>(M_PI)},
                   { 0.25f * static_cast<float>(M_PI), 1.15f * static_cast<float>(M_PI)}};
    m_waves.append(w);

    for (int i = 0; i < 8; i++) {
        float a = dis(m_rng) * static_cast<float>(M_PI) * 2.0f;
        m_sparks.append({
            x, y, std::cos(a) * (4.8f + dis(m_rng) * 2.0f), std::sin(a) * (4.8f + dis(m_rng) * 2.0f),
            dis(m_rng) * static_cast<float>(M_PI) * 2.0f, (dis(m_rng) - 0.5f) * 0.28f,
            (4.0f + dis(m_rng) * 3.0f) * m_scale, 0.9f, 1.0f
        });
    }
}

void BasparkEffect::repaint()
{
    if (isActive()) effects->addRepaintFull();
}

static void helperDraw(const QList<QVector2D> &verts, GLenum mode)
{
    GLVertexBuffer *vbo = GLVertexBuffer::streamingBuffer();
    vbo->reset();
    vbo->setData(verts.constData(), verts.size() * sizeof(QVector2D));
    vbo->setVertexCount(verts.size());
    static const GLVertexAttrib layout[] = {{VA_Position, 2, GL_FLOAT, 0}};
    vbo->setAttribLayout(std::span(layout), sizeof(QVector2D));
    vbo->render(mode);
}

void BasparkEffect::drawTrail(const RenderViewport &viewport)
{
    if (m_trail.size() < 2) return;
    const float s = viewport.scale();
    QList<QVector2D> v;
    for (const auto &p : m_trail) v.append(QVector2D(p.pos.x() * s, p.pos.y() * s));
    
    for (int i = 0; i < 4; i++) {
        float w = 4.5f + (10.0f - 4.5f) * (i / 3.0f);
        QColor c = m_color;
        c.setAlphaF(0.35f * (1.0f - (i / 3.0f) * 0.7f));
        ShaderManager::instance()->getBoundShader()->setUniform(GLShader::ColorUniform::Color, c);
        glLineWidth(w * s);
        helperDraw(v, GL_LINE_STRIP);
    }
}

void BasparkEffect::drawWave(const RenderViewport &viewport, const Wave &w)
{
    const float s = viewport.scale();
    float alpha = std::max(0.0f, 1.0f - w.life / w.maxLife);
    if (alpha > 0) {
        QList<QVector2D> v; v.append(QVector2D(w.x * s, w.y * s));
        for (int i = 0; i <= 40; i++) {
            float a = 2.0f * M_PI * i / 40.0f;
            v.append(QVector2D((w.x + w.r * std::cos(a)) * s, (w.y + w.r * std::sin(a)) * s));
        }
        QColor c = m_color; c.setAlphaF(alpha * 0.45f);
        ShaderManager::instance()->getBoundShader()->setUniform(GLShader::ColorUniform::Color, c);
        helperDraw(v, GL_TRIANGLE_FAN);
    }

    float rProg = std::min(w.ring.life / w.ring.maxLife, 1.0f);
    for (const auto &seg : w.ring.segs) {
        float len = seg.len * std::max(0.0f, 1.0f - rProg);
        float start = w.ring.ang + seg.off;
        QList<QVector2D> v;
        for (int i = 0; i <= 24; i++) {
            float a = start + len * i / 24.0f;
            float r = (w.r + 3.0f * m_scale);
            v.append(QVector2D((w.x + r * std::cos(a)) * s, (w.y + r * std::sin(a)) * s));
        }
        for (int i = 0; i < 3; i++) {
            QColor c(245, 248, 252);
            c.setAlphaF((1.0f - rProg) * (1.0f - (i / 2.0f) * 0.5f));
            ShaderManager::instance()->getBoundShader()->setUniform(GLShader::ColorUniform::Color, c);
            glLineWidth((3.7f + i * 1.15f) * s);
            helperDraw(v, GL_LINE_STRIP);
        }
    }
}

void BasparkEffect::drawSpark(const RenderViewport &viewport, const Spark &s)
{
    const float sc = viewport.scale();
    float cR = std::cos(s.rot), sR = std::sin(s.rot);
    auto rot = [&](float px, float py) {
        return QVector2D((s.x + (px * cR - py * sR)) * sc, (s.y + (px * sR + py * cR)) * sc);
    };
    QColor c = Qt::white; c.setAlphaF(s.a);
    ShaderManager::instance()->getBoundShader()->setUniform(GLShader::ColorUniform::Color, c);
    helperDraw({rot(0, -s.s), rot(s.s * 0.6f, s.s * 0.6f), rot(-s.s * 0.6f, s.s * 0.6f)}, GL_TRIANGLES);
}

void BasparkEffect::setupGL(const RenderTarget &renderTarget, const QMatrix4x4 &proj)
{
    GLShader *sh = ShaderManager::instance()->pushShader(ShaderTrait::UniformColor | ShaderTrait::TransformColorspace);
    sh->setUniform(GLShader::Mat4Uniform::ModelViewProjectionMatrix, proj);
    sh->setColorspaceUniforms(ColorDescription::sRGB, renderTarget.colorDescription(), RenderingIntent::Perceptual);
    GLint blendSrc, blendDst;
    glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrc);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendDst);
    m_savedBlendSrc = blendSrc;
    m_savedBlendDst = blendDst;
    m_savedBlend = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Exact Canvas "lighter"
}

void BasparkEffect::finalizeGL()
{
    glBlendFunc(m_savedBlendSrc, m_savedBlendDst);
    if (!m_savedBlend) glDisable(GL_BLEND);
    ShaderManager::instance()->popShader();
}

} // namespace KWin

#include "moc_baspark.cpp"
