#include "pad_manager.h"
#include "texture_manager.h"
#include "sprite_renderer.h"
#include <cmath>
#include <algorithm>

namespace azure_mirage {

PadManager& PadManager::getInstance() {
    static PadManager instance;
    return instance;
}

PadManager::PadManager()
    : m_mode(PadMode::MENU)
    , m_mask(0)
    , m_latchedMask(0)
    , m_touching(false)
    , m_touchX(-1.0f)
    , m_touchY(-1.0f)
    , m_screenW(1280.0f)
    , m_screenH(720.0f)
    , m_vpX(0.0f)
    , m_vpY(0.0f)
    , m_vpW(1280.0f)
    , m_vpH(720.0f)
    , m_dpadDrawX(20.0f)
    , m_dpadDrawY(444.0f)
    , m_dpadDrawSize(256.0f)
    , m_dpadCenterX(148.0f)
    , m_dpadCenterY(572.0f)
    , m_dpadTouchRadius(200.0f)
    , m_startDrawX(616.0f)
    , m_startDrawY(654.0f)
    , m_startDrawSize(48.0f)
    , m_startCenterX(640.0f)
    , m_startCenterY(678.0f)
    , m_startTouchRadius(42.0f)
    , m_texturesLoaded(false)
    , m_dpadTex(0)
    , m_dpadW(0)
    , m_dpadH(0)
    , m_startTex(0)
    , m_startW(0)
    , m_startH(0)
{
    m_actionButtons[0] = {1006.0f, 539.0f, 96.0f, 1054.0f, 587.0f, 72.0f, BTN_ATTACK, 0, 0, 0, true};
    m_actionButtons[1] = {1090.0f, 614.0f, 96.0f, 1138.0f, 662.0f, 72.0f, BTN_JUMP,   0, 0, 0, true};
    m_actionButtons[2] = {1174.0f, 539.0f, 96.0f, 1222.0f, 587.0f, 72.0f, BTN_DASH,   0, 0, 0, true};
    m_actionButtons[3] = {1090.0f, 462.0f, 96.0f, 1138.0f, 510.0f, 72.0f, BTN_SKILL,  0, 0, 0, true};
    m_actionButtons[4] = {1187.0f, 262.0f, 76.0f, 1225.0f, 300.0f, 64.0f, BTN_SUPER,  0, 0, 0, false};
    m_actionButtons[5] = {1009.0f, 262.0f, 76.0f, 1047.0f, 300.0f, 64.0f, BTN_ASSIST, 0, 0, 0, false};
    m_actionButtons[6] = {18.0f,   262.0f, 76.0f, 56.0f,   300.0f, 64.0f, BTN_BURST,  0, 0, 0, false};

    m_arcadeButtons[0] = {960.0f,  440.0f, 80.0f, 1000.0f, 480.0f, 50.0f, BTN_X, 0, 0, 0, true};
    m_arcadeButtons[1] = {1055.0f, 415.0f, 80.0f, 1095.0f, 455.0f, 50.0f, BTN_Y, 0, 0, 0, true};
    m_arcadeButtons[2] = {1150.0f, 395.0f, 80.0f, 1190.0f, 435.0f, 50.0f, BTN_Z, 0, 0, 0, false};
    m_arcadeButtons[3] = {960.0f,  560.0f, 80.0f, 1000.0f, 600.0f, 50.0f, BTN_A, 0, 0, 0, true};
    m_arcadeButtons[4] = {1055.0f, 535.0f, 80.0f, 1095.0f, 575.0f, 50.0f, BTN_B, 0, 0, 0, true};
    m_arcadeButtons[5] = {1150.0f, 515.0f, 80.0f, 1190.0f, 555.0f, 50.0f, BTN_C, 0, 0, 0, false};
}

void PadManager::setScreenSize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    m_screenW = static_cast<float>(width);
    m_screenH = static_cast<float>(height);

    float targetAspect = 1280.0f / 720.0f;
    float screenAspect = m_screenW / m_screenH;

    if (screenAspect >= targetAspect) {
        m_vpH = m_screenH;
        m_vpW = m_screenH * targetAspect;
        m_vpX = (m_screenW - m_vpW) * 0.5f;
        m_vpY = 0.0f;
    } else {
        m_vpW = m_screenW;
        m_vpH = m_screenW / targetAspect;
        m_vpX = 0.0f;
        m_vpY = (m_screenH - m_vpH) * 0.5f;
    }
}

bool PadManager::handleInput(const AInputEvent* event) {
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) {
        return false;
    }

    int32_t action = AMotionEvent_getAction(event);
    int32_t actionMasked = action & AMOTION_EVENT_ACTION_MASK;
    size_t pointerCount = AMotionEvent_getPointerCount(event);

    if (actionMasked == AMOTION_EVENT_ACTION_UP || actionMasked == AMOTION_EVENT_ACTION_CANCEL) {
        m_touching = false;
        m_mask = 0;
        m_touchX = -1.0f;
        m_touchY = -1.0f;
        return true;
    }

    m_touching = true;

    float invVpW = (m_vpW > 0.0f) ? (1280.0f / m_vpW) : 1.0f;
    float invVpH = (m_vpH > 0.0f) ? (720.0f / m_vpH) : 1.0f;

    size_t primaryIdx = 0;
    if (actionMasked == AMOTION_EVENT_ACTION_POINTER_UP) {
        size_t actionIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        if (primaryIdx == actionIndex && pointerCount > 1) {
            primaryIdx = 1;
        }
    }

    float raw0X = AMotionEvent_getX(event, primaryIdx);
    float raw0Y = AMotionEvent_getY(event, primaryIdx);
    float cl0X = std::max(m_vpX, std::min(m_vpX + m_vpW, raw0X));
    float cl0Y = std::max(m_vpY, std::min(m_vpY + m_vpH, raw0Y));
    m_touchX = (cl0X - m_vpX) * invVpW;
    m_touchY = (cl0Y - m_vpY) * invVpH;

    if (m_mode == PadMode::HIDDEN) {
        m_mask = 0;
        m_latchedMask = 0;
        return false;
    }

    uint32_t newMask = 0;

    for (size_t i = 0; i < pointerCount; i++) {
        if (actionMasked == AMOTION_EVENT_ACTION_POINTER_UP) {
            size_t actionIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            if (i == actionIndex) continue;
        }

        float rawX = AMotionEvent_getX(event, i);
        float rawY = AMotionEvent_getY(event, i);
        float clampedX = std::max(m_vpX, std::min(m_vpX + m_vpW, rawX));
        float clampedY = std::max(m_vpY, std::min(m_vpY + m_vpH, rawY));
        float tx = (clampedX - m_vpX) * invVpW;
        float ty = (clampedY - m_vpY) * invVpH;

        bool inDpadArea = (tx <= 500.0f && ty >= 340.0f);
        if (inDpadArea) {
            float ddx = tx - m_dpadCenterX;
            float ddy = ty - m_dpadCenterY;
            if (ddx * ddx + ddy * ddy <= 240.0f * 240.0f) {
                constexpr float DEADZONE = 25.0f;
                if (ddy < -DEADZONE) {
                    newMask |= BTN_UP;
                } else if (ddy > DEADZONE) {
                    newMask |= BTN_DOWN;
                }

                if (ddx < -DEADZONE) {
                    newMask |= BTN_LEFT;
                } else if (ddx > DEADZONE) {
                    newMask |= BTN_RIGHT;
                }
            }
        } else if (m_mode == PadMode::BATTLE && m_type == PadType::ARCADE) {
            float bestDistSq = 999999.0f;
            uint32_t bestFlag = 0;
            for (int b = 0; b < 6; ++b) {
                const auto& btn = m_arcadeButtons[b];
                float bdx = tx - btn.centerX;
                float bdy = ty - btn.centerY;
                float distSq = bdx * bdx + bdy * bdy;
                float radSq = btn.touchRadius * btn.touchRadius;
                if (distSq <= radSq && distSq < bestDistSq) {
                    bestDistSq = distSq;
                    bestFlag = btn.flag;
                }
            }
            if (bestFlag != 0) {
                newMask |= bestFlag;
            }
        } else {
            float bestDistSq = 999999.0f;
            uint32_t bestFlag = 0;
            for (int b = 0; b < 7; ++b) {
                const auto& btn = m_actionButtons[b];
                if (m_mode == PadMode::MENU && !btn.inMenu) continue;
                float bdx = tx - btn.centerX;
                float bdy = ty - btn.centerY;
                float distSq = bdx * bdx + bdy * bdy;
                float radSq = btn.touchRadius * btn.touchRadius;
                if (distSq <= radSq && distSq < bestDistSq) {
                    bestDistSq = distSq;
                    bestFlag = btn.flag;
                }
            }
            if (bestFlag != 0) {
                newMask |= bestFlag;
            }
        }

        float sdx = tx - m_startCenterX;
        float sdy = ty - m_startCenterY;
        if (sdx * sdx + sdy * sdy <= m_startTouchRadius * m_startTouchRadius) {
            newMask |= BTN_START;
        }
    }

    m_mask = newMask;
    m_latchedMask |= newMask;
    return true;
}

void PadManager::ensureTextures() {
    if (m_texturesLoaded) return;

    auto dpadInfo = TextureManager::getInstance().loadTexture("joy/arrow.png");
    m_dpadTex = dpadInfo.id;
    m_dpadW = dpadInfo.width;
    m_dpadH = dpadInfo.height;

    auto startInfo = TextureManager::getInstance().loadTexture("joy/start.png");
    m_startTex = startInfo.id;
    m_startW = startInfo.width;
    m_startH = startInfo.height;

    const char* actionFiles[7] = {
        "joy/X.png",
        "joy/A.png",
        "joy/B.png",
        "joy/Y.png",
        "joy/R.png",
        "joy/r2.png",
        "joy/L.png"
    };

    for (int i = 0; i < 7; ++i) {
        auto info = TextureManager::getInstance().loadTexture(actionFiles[i]);
        m_actionButtons[i].tex = info.id;
        m_actionButtons[i].texW = info.width;
        m_actionButtons[i].texH = info.height;
    }

    const char* arcadeFiles[6] = {
        "joy/X.png",
        "joy/Y.png",
        "joy/r2.png",
        "joy/A.png",
        "joy/B.png",
        "joy/R.png"
    };

    for (int i = 0; i < 6; ++i) {
        auto info = TextureManager::getInstance().loadTexture(arcadeFiles[i]);
        m_arcadeButtons[i].tex = info.id;
        m_arcadeButtons[i].texW = info.width;
        m_arcadeButtons[i].texH = info.height;
    }

    if (m_dpadTex != 0 && m_startTex != 0 && m_actionButtons[0].tex != 0) {
        m_texturesLoaded = true;
    }
}

void PadManager::render(SpriteRenderer* renderer, float alpha) {
    if (!renderer || alpha <= 0.01f || m_mode == PadMode::HIDDEN) return;

    ensureTextures();

    renderer->begin();

    if (m_dpadTex != 0) {
        bool dpadPressed = (m_mask & (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT)) != 0;
        float dpadScale = dpadPressed ? 1.05f : 1.0f;
        float drawSize = m_dpadDrawSize * dpadScale;
        float px = m_dpadCenterX - drawSize * 0.5f;
        float py = m_dpadCenterY - drawSize * 0.5f;
        float a = dpadPressed ? std::min(1.0f, alpha * 1.35f) : alpha;
        renderer->drawSprite(
            m_dpadTex,
            px, py,
            0, 0, m_dpadW, m_dpadH,
            drawSize, drawSize,
            0, 0, 1,
            1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, a
        );
    }

    if (m_mode == PadMode::BATTLE && m_type == PadType::ARCADE) {
        for (const auto& btn : m_arcadeButtons) {
            if (btn.tex == 0) continue;

            bool pressed = (m_mask & btn.flag) != 0;
            float btnScale = pressed ? 1.15f : 1.0f;
            float drawSize = btn.size * btnScale;
            float px = btn.centerX - drawSize * 0.5f;
            float py = btn.centerY - drawSize * 0.5f;
            float a = pressed ? std::min(1.0f, alpha * 1.35f) : alpha;

            renderer->drawSprite(
                btn.tex,
                px, py,
                0, 0, btn.texW, btn.texH,
                drawSize, drawSize,
                0, 0, 1,
                1.0f, 1.0f,
                1.0f, 1.0f, 1.0f, a
            );
        }
    } else {
        for (const auto& btn : m_actionButtons) {
            if (btn.tex == 0) continue;
            if (m_mode == PadMode::MENU && !btn.inMenu) continue;

            bool pressed = (m_mask & btn.flag) != 0;
            float btnScale = pressed ? 1.15f : 1.0f;
            float drawSize = btn.size * btnScale;
            float px = btn.centerX - drawSize * 0.5f;
            float py = btn.centerY - drawSize * 0.5f;
            float a = pressed ? std::min(1.0f, alpha * 1.35f) : alpha;

            renderer->drawSprite(
                btn.tex,
                px, py,
                0, 0, btn.texW, btn.texH,
                drawSize, drawSize,
                0, 0, 1,
                1.0f, 1.0f,
                1.0f, 1.0f, 1.0f, a
            );
        }
    }

    if (m_startTex != 0) {
        bool pressed = (m_mask & BTN_START) != 0;
        float startScale = pressed ? 1.15f : 1.0f;
        float drawSize = m_startDrawSize * startScale;
        float px = m_startCenterX - drawSize * 0.5f;
        float py = m_startCenterY - drawSize * 0.5f;
        float a = pressed ? std::min(1.0f, alpha * 1.35f) : alpha * 0.85f;
        renderer->drawSprite(
            m_startTex,
            px, py,
            0, 0, m_startW, m_startH,
            drawSize, drawSize,
            0, 0, 1,
            1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, a
        );
    }

    renderer->end();
}

}
