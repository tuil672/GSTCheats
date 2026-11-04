#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/cocos/draw_nodes/CCDrawNode.h>

#include <cstdlib>
#include <string>

using namespace geode::prelude;

namespace gst {
    // ---------------------------------------------------------------------
    // Persistent state
    // ---------------------------------------------------------------------
    bool noclipEnabled = false;
    bool speedEnabled = false;
    bool autoRestart = false;
    bool forcePractice = false;
    bool hitboxes = false;
    bool hudEnabled = true;
    bool showFps = true;
    bool showPercent = true;

    int speedIndex = 2;
    int themeIndex = 0;
    int difficultyIndex = 1;
    int lengthIndex = 1;
    int decorIndex = 2;
    int modeIndex = 0;
    int styleIndex = 0;
    int variantIndex = 0;

    constexpr const char* difficulties[] = { "EASY", "NORMAL", "HARD", "DEMON" };
    constexpr const char* lengths[] = { "SHORT", "MEDIUM", "LONG" };
    constexpr const char* decorLevels[] = { "OFF", "LIGHT", "FULL" };
    constexpr const char* styles[] = { "FLOW", "TOWERS", "CORRIDOR" };
    constexpr const char* speedNames[] = { "1.0x", "1.25x", "1.5x", "2.0x", "3.0x" };
    constexpr float speeds[] = { 1.0f, 1.25f, 1.5f, 2.0f, 3.0f };

    // 0 = MIX, then one entry per game mode.
    constexpr const char* modeNames[] = {
        "MIX", "CUBE", "SHIP", "BALL", "UFO", "WAVE", "ROBOT", "SPIDER", "SWING"
    };
    constexpr int modePortalIDs[] = {
        0, 12, 13, 47, 111, 660, 745, 1331, 1933
    };

    struct Theme {
        const char* name;
        ccColor3B primary;
        ccColor3B accent;
        ccColor3B detail;
    };

    Theme themes[] = {
        { "NEON",   {  61, 203, 255 }, { 255,  74, 185 }, { 112, 115, 255 } },
        { "SUNSET", { 255, 126,  91 }, { 255,  75, 151 }, { 185,  91, 184 } },
        { "FOREST", {  92, 220, 147 }, {  49, 165, 142 }, { 175, 242, 130 } },
        { "GOLD",   { 255, 204,  76 }, { 255, 145,  58 }, { 255, 238, 151 } },
        { "ICE",    { 120, 220, 255 }, { 180, 245, 255 }, {  90, 150, 255 } },
    };

    constexpr int themeCount = 5;

    int clampIndex(int value, int count) {
        if (value < 0) return 0;
        if (value >= count) return count - 1;
        return value;
    }

    bool isFlyingMode(int mode) {
        // SHIP, UFO, WAVE, SWING
        return mode == 2 || mode == 4 || mode == 5 || mode == 8;
    }

    void loadSettings() {
        auto* mod = Mod::get();
        if (!mod) return;

        noclipEnabled = mod->getSavedValue<bool>("noclip", false);
        speedEnabled = mod->getSavedValue<bool>("speedhack", false);
        autoRestart = mod->getSavedValue<bool>("autoRestart", false);
        forcePractice = mod->getSavedValue<bool>("forcePractice", false);
        hitboxes = mod->getSavedValue<bool>("hitboxes", false);
        hudEnabled = mod->getSavedValue<bool>("hud", true);
        showFps = mod->getSavedValue<bool>("showFps", true);
        showPercent = mod->getSavedValue<bool>("showPercent", true);

        speedIndex = clampIndex(mod->getSavedValue<int>("speedIndex", 2), 5);
        themeIndex = clampIndex(mod->getSavedValue<int>("themeIndex", 0), themeCount);
        difficultyIndex = clampIndex(mod->getSavedValue<int>("difficultyIndex", 1), 4);
        lengthIndex = clampIndex(mod->getSavedValue<int>("lengthIndex", 1), 3);
        decorIndex = clampIndex(mod->getSavedValue<int>("decorIndex", 2), 3);
        modeIndex = clampIndex(mod->getSavedValue<int>("modeIndex", 0), 9);
        styleIndex = clampIndex(mod->getSavedValue<int>("styleIndex", 0), 3);
        variantIndex = clampIndex(mod->getSavedValue<int>("variantIndex", 0), 4);
    }

    void saveSettings() {
        auto* mod = Mod::get();
        if (!mod) return;

        mod->setSavedValue<bool>("noclip", noclipEnabled);
        mod->setSavedValue<bool>("speedhack", speedEnabled);
        mod->setSavedValue<bool>("autoRestart", autoRestart);
        mod->setSavedValue<bool>("forcePractice", forcePractice);
        mod->setSavedValue<bool>("hitboxes", hitboxes);
        mod->setSavedValue<bool>("hud", hudEnabled);
        mod->setSavedValue<bool>("showFps", showFps);
        mod->setSavedValue<bool>("showPercent", showPercent);

        mod->setSavedValue<int>("speedIndex", speedIndex);
        mod->setSavedValue<int>("themeIndex", themeIndex);
        mod->setSavedValue<int>("difficultyIndex", difficultyIndex);
        mod->setSavedValue<int>("lengthIndex", lengthIndex);
        mod->setSavedValue<int>("decorIndex", decorIndex);
        mod->setSavedValue<int>("modeIndex", modeIndex);
        mod->setSavedValue<int>("styleIndex", styleIndex);
        mod->setSavedValue<int>("variantIndex", variantIndex);
    }

    void notify(const char* message) {
        FLAlertLayer::create("GSTgeode", message, "OK")->show();
    }

    void openPanel();

    // ---------------------------------------------------------------------
    // Level generator
    //
    // Grid notes:
    //  * GD's editor grid is 30 units per cell.
    //  * Ground top is y = 90, so a ground object's centre is y = 105.
    //  * Decoration lives at y >= 315, far above the player's jump height.
    // ---------------------------------------------------------------------
    bool buildLayout() {
        auto* editor = LevelEditorLayer::get();
        if (!editor || !editor->m_editorUI) {
            notify("Open a level in the editor first, then press GENERATE LEVEL.");
            return false;
        }

        auto* ui = editor->m_editorUI;
        const Theme& theme = themes[clampIndex(themeIndex, themeCount)];

        // Cell counts are deliberately much larger than v1.x: 30 units per cell
        // means the old 32/48/72 values were only 3-7 seconds of gameplay.
        const int segmentsByLength[] = { 160, 280, 420 };
        const int spikeEveryByDifficulty[] = { 9, 7, 5, 4 };
        const int blockEveryByDifficulty[] = { 14, 11, 8, 6 };

        const int segments = segmentsByLength[clampIndex(lengthIndex, 3)];
        const int difficulty = clampIndex(difficultyIndex, 4);
        const int spikeEvery = spikeEveryByDifficulty[difficulty];
        const int blockEvery = blockEveryByDifficulty[difficulty];

        const int sectionCount = segments >= 300 ? 8 : (segments >= 200 ? 7 : 6);
        const int sectionLen = segments / sectionCount;
        const int mixOrder[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        const int selectedMode = clampIndex(modeIndex, 9);

        const float grid = 30.f;
        const float startX = 180.f;
        const float groundY = 105.f;
        const float decorY = 315.f;

        int placed = 0;

        auto add = [&](int objectID, CCPoint position, ccColor3B color,
                       float scale = 1.f, float rotation = 0.f,
                       unsigned char opacity = 255) {
            if (auto* object = ui->createObject(objectID, position)) {
                object->setObjectColor(color);
                if (scale != 1.f) object->setScale(scale);
                if (rotation != 0.f) object->setRotation(rotation);
                if (opacity != 255) object->setOpacity(opacity);
                ++placed;
            }
        };

        // -- Gameplay ------------------------------------------------------
        // A portal stack of three copies covers the full vertical range
        // (60..330), so a mode switch is guaranteed no matter how high the
        // player is when leaving a flying section.
        auto addPortalStack = [&](int mode, float x) {
            if (mode <= 0) return;
            for (int level = 0; level < 3; ++level) {
                add(modePortalIDs[mode], { x, groundY + static_cast<float>(level) * grid * 3.f },
                    theme.accent);
            }
        };

        for (int section = 0; section < sectionCount; ++section) {
            const int mode = (selectedMode == 0) ? mixOrder[section % 8] : selectedMode;
            const float sectionX = startX + static_cast<float>(section) * sectionLen * grid;
            const bool flying = isFlyingMode(mode);

            if (section > 0) addPortalStack(mode, sectionX);

            // Keep the cells around portals clear so the transition is safe.
            const int firstCell = section > 0 ? 3 : 4;
            const int lastCell = sectionLen - 2;

            if (flying) {
                if (mode == 5) { // WAVE
                    for (int i = firstCell; i < lastCell; ++i) {
                        const float x = sectionX + static_cast<float>(i) * grid;
                        add(1, { x, 255.f }, theme.detail, 1.f, 0.f, 230);

                        const int pillarEvery = (difficulty >= 3) ? 7 : 9;
                        if ((i + variantIndex) % pillarEvery == 0) {
                            add(1, { x, groundY }, theme.primary);
                            add(1, { x, groundY + grid }, theme.primary);
                            add(1, { x, groundY + grid * 2.f }, theme.primary);
                        }
                    }
                } else if (mode == 8) { // SWING
                    for (int i = firstCell; i < lastCell; ++i) {
                        const float x = sectionX + static_cast<float>(i) * grid;
                        add(1, { x, 285.f }, theme.detail, 1.f, 0.f, 230);
                        if ((i + variantIndex) % 6 == 0) add(1, { x, 165.f }, theme.accent);
                        if ((i + variantIndex) % 6 == 3) add(1, { x, 225.f }, theme.accent);
                    }
                } else if (mode == 2) { // SHIP
                    for (int i = firstCell; i < lastCell; ++i) {
                        const float x = sectionX + static_cast<float>(i) * grid;
                        add(1, { x, 255.f }, theme.detail, 1.f, 0.f, 230);
                        if ((i + variantIndex) % 5 == 0) add(1, { x, groundY }, theme.primary);
                        if ((i + variantIndex) % 5 == 3) add(1, { x, 195.f }, theme.accent);
                    }
                } else { // UFO
                    for (int i = firstCell; i < lastCell; ++i) {
                        const float x = sectionX + static_cast<float>(i) * grid;
                        if (i % 2 == 0) add(1, { x, 255.f }, theme.detail, 1.f, 0.f, 230);
                        if ((i + variantIndex) % 4 == 0) add(1, { x, 165.f }, theme.accent);
                        if ((i + variantIndex) % 8 == 0) add(8, { x, groundY }, theme.accent);
                    }
                }
            } else if (mode == 3) { // BALL
                for (int i = firstCell; i < lastCell; ++i) {
                    const float x = sectionX + static_cast<float>(i) * grid;

                    if ((i + variantIndex) % 5 == 0) add(1, { x, groundY }, theme.primary);
                    if ((i + variantIndex) % 5 == 2) add(1, { x, 225.f }, theme.detail);
                    if ((i + variantIndex) % 9 == 0) add(8, { x, groundY }, theme.accent);
                    if (styleIndex == 1 && i % 7 == 3) add(1, { x, 255.f }, theme.detail, 1.f, 0.f, 220);
                }
            } else { // CUBE, ROBOT, SPIDER
                for (int i = firstCell; i < lastCell; ++i) {
                    const float x = sectionX + static_cast<float>(i) * grid;
                    const int phase = (variantIndex + section) % 5;

                    if ((i + phase) % spikeEvery == 0) {
                        add(8, { x, groundY }, theme.accent);
                        if (difficulty >= 2 && (i + phase) % (spikeEvery * 2) == 0) {
                            add(8, { x + grid, groundY }, theme.accent);
                        }
                    } else if ((i + phase) % blockEvery == 0) {
                        // Never stack more than two blocks: a three-block wall
                        // is not jumpable by the cube.
                        add(1, { x, groundY }, theme.primary);
                        if (difficulty >= 2) add(1, { x, groundY + grid }, theme.primary);
                    }

                    if (styleIndex == 1 && i % 7 == 3) {
                        add(1, { x, 255.f }, theme.detail, 1.f, 0.f, 220);
                    }
                    if (styleIndex == 2 && i % 4 == 0) {
                        add(1, { x, 225.f }, theme.detail, 1.f, 0.f, 220);
                    }
                }
            }
        }

        // -- Decoration ----------------------------------------------------
        if (decorIndex > 0) {
            const int diamondStep = (decorIndex >= 2) ? 2 : 4;

            // Floating diamonds with a slight height variation.
            for (int i = 0; i < segments; i += diamondStep) {
                const float x = startX + static_cast<float>(i) * grid;
                const int layer = (i / diamondStep) % 3;
                const float y = decorY + static_cast<float>(layer) * 30.f;
                add(1, { x, y }, (layer == 1 ? theme.accent : theme.detail), 0.5f, 45.f, 170);
            }

            // Thin light beams.
            for (int i = 5; i < segments; i += 12) {
                const float x = startX + static_cast<float>(i) * grid;
                if (auto* beam = ui->createObject(1, { x, decorY + 75.f })) {
                    beam->setObjectColor(theme.accent);
                    beam->setScaleX(0.12f);
                    beam->setScaleY(3.f);
                    beam->setOpacity(90);
                    ++placed;
                }
            }

            // Distant stars.
            for (int i = 1; i < segments; i += 5) {
                const float x = startX + static_cast<float>(i) * grid;
                add(1, { x, decorY + 165.f }, theme.detail, 0.28f, 45.f, 200);
            }

            if (decorIndex >= 2) {
                // Arches made from scaled blocks.
                for (int i = 3; i < segments; i += 10) {
                    const float x = startX + static_cast<float>(i) * grid;

                    for (int row = 0; row < 3; ++row) {
                        add(1, { x, decorY + static_cast<float>(row) * 30.f },
                            (row % 2 == 0) ? theme.primary : theme.detail, 0.75f, 0.f, 220);
                        add(1, { x + grid * 4.f, decorY + static_cast<float>(row) * 30.f },
                            (row % 2 == 0) ? theme.primary : theme.detail, 0.75f, 0.f, 220);
                    }

                    for (int beam = 0; beam < 5; ++beam) {
                        add(1, { x + static_cast<float>(beam) * grid, decorY + 90.f },
                            (beam % 2 == 0) ? theme.detail : theme.accent, 0.75f, 0.f, 220);
                    }
                }
            }
        }

        const std::string message = fmt::format(
            "GSTgeode: {} objects | {} | {} | {} | {} | {}",
            placed,
            modeNames[clampIndex(modeIndex, 9)],
            themes[clampIndex(themeIndex, themeCount)].name,
            difficulties[difficulty],
            lengths[clampIndex(lengthIndex, 3)],
            styles[clampIndex(styleIndex, 3)]
        );
        log::info("GSTgeode generated {} objects", placed);
        notify(message.c_str());
        return true;
    }
}

// -------------------------------------------------------------------------
// Control panel with tabs
// -------------------------------------------------------------------------
class GSTControlPanel : public Popup {
protected:
    int m_tab = 0;

    CCMenu* m_playerPage = nullptr;
    CCMenu* m_builderPage = nullptr;
    CCMenu* m_visualPage = nullptr;

    CCLabelBMFont* m_tabLabels[3] = { nullptr, nullptr, nullptr };
    CCLabelBMFont* m_hint = nullptr;

    CCLabelBMFont* m_noclipLabel = nullptr;
    CCLabelBMFont* m_speedLabel = nullptr;
    CCLabelBMFont* m_rateLabel = nullptr;
    CCLabelBMFont* m_restartLabel = nullptr;
    CCLabelBMFont* m_practiceLabel = nullptr;

    CCLabelBMFont* m_modeLabel = nullptr;
    CCLabelBMFont* m_themeLabel = nullptr;
    CCLabelBMFont* m_difficultyLabel = nullptr;
    CCLabelBMFont* m_lengthLabel = nullptr;
    CCLabelBMFont* m_decorLabel = nullptr;
    CCLabelBMFont* m_styleLabel = nullptr;
    CCLabelBMFont* m_variantLabel = nullptr;

    CCLabelBMFont* m_hitboxLabel = nullptr;
    CCLabelBMFont* m_hudLabel = nullptr;
    CCLabelBMFont* m_fpsLabel = nullptr;
    CCLabelBMFont* m_percentLabel = nullptr;

    static constexpr ccColor3B kOn = { 92, 235, 150 };
    static constexpr ccColor3B kOff = { 150, 165, 190 };
    static constexpr ccColor3B kValue = { 255, 213, 120 };
    static constexpr ccColor3B kTitle = { 205, 224, 245 };

    CCLabelBMFont* addRow(
        CCMenu* menu,
        const char* title,
        const char* value,
        CCPoint position,
        SEL_MenuHandler callback,
        float width = 390.f
    ) {
        auto* normal = CCLayerColor::create(ccColor4B{ 20, 38, 66, 255 }, width, 28.f);
        auto* selected = CCLayerColor::create(ccColor4B{ 30, 60, 100, 255 }, width, 28.f);
        auto* item = CCMenuItemSprite::create(normal, selected, this, callback);
        if (!item) return nullptr;
        item->setPosition(position);
        menu->addChild(item);

        auto* titleLabel = CCLabelBMFont::create(title, "bigFont.fnt");
        titleLabel->setAnchorPoint({ 0.f, 0.5f });
        // Children of a CCMenuItem use the item's bottom-left corner as origin,
        // so the labels must live inside 0..width.
        titleLabel->setPosition({ 12.f, 14.f });
        titleLabel->setScale(0.34f);
        titleLabel->setColor(kTitle);
        item->addChild(titleLabel);

        auto* valueLabel = CCLabelBMFont::create(value, "bigFont.fnt");
        valueLabel->setAnchorPoint({ 1.f, 0.5f });
        valueLabel->setPosition({ width - 12.f, 14.f });
        valueLabel->setScale(0.34f);
        valueLabel->setColor(kValue);
        item->addChild(valueLabel);
        return valueLabel;
    }

    CCMenuItemSpriteExtra* addAction(
        CCMenu* menu,
        const char* text,
        CCPoint position,
        SEL_MenuHandler callback,
        float scale = 0.7f
    ) {
        auto* sprite = ButtonSprite::create(text, "goldFont.fnt", "GJ_button_01.png", scale);
        auto* item = CCMenuItemSpriteExtra::create(sprite, this, callback);
        item->setPosition(position);
        menu->addChild(item);
        return item;
    }

    CCMenu* makePage() {
        auto* menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        m_mainLayer->addChild(menu, 5);
        return menu;
    }

    void setValue(CCLabelBMFont* label, const std::string& text, ccColor3B color) {
        if (!label) return;
        label->setString(text.c_str());
        label->setColor(color);
    }

    void refreshLabels() {
        setValue(m_noclipLabel, gst::noclipEnabled ? "ON" : "OFF", gst::noclipEnabled ? kOn : kOff);
        setValue(m_speedLabel, gst::speedEnabled ? "ON" : "OFF", gst::speedEnabled ? kOn : kOff);
        setValue(m_rateLabel, gst::speedNames[gst::speedIndex], kValue);
        setValue(m_restartLabel, gst::autoRestart ? "ON" : "OFF", gst::autoRestart ? kOn : kOff);
        setValue(m_practiceLabel, gst::forcePractice ? "ON" : "OFF", gst::forcePractice ? kOn : kOff);

        setValue(m_modeLabel, gst::modeNames[gst::modeIndex], kValue);
        setValue(m_themeLabel, gst::themes[gst::themeIndex].name, kValue);
        setValue(m_difficultyLabel, gst::difficulties[gst::difficultyIndex], kValue);
        setValue(m_lengthLabel, gst::lengths[gst::lengthIndex], kValue);
        setValue(m_decorLabel, gst::decorLevels[gst::decorIndex], kValue);
        setValue(m_styleLabel, gst::styles[gst::styleIndex], kValue);
        setValue(m_variantLabel, fmt::format("V{}", gst::variantIndex + 1), kValue);

        setValue(m_hitboxLabel, gst::hitboxes ? "ON" : "OFF", gst::hitboxes ? kOn : kOff);
        setValue(m_hudLabel, gst::hudEnabled ? "ON" : "OFF", gst::hudEnabled ? kOn : kOff);
        setValue(m_fpsLabel, gst::showFps ? "ON" : "OFF", gst::showFps ? kOn : kOff);
        setValue(m_percentLabel, gst::showPercent ? "ON" : "OFF", gst::showPercent ? kOn : kOff);
    }

    void setTab(int tab) {
        m_tab = tab;
        if (m_playerPage) m_playerPage->setVisible(tab == 0);
        if (m_builderPage) m_builderPage->setVisible(tab == 1);
        if (m_visualPage) m_visualPage->setVisible(tab == 2);

        for (int i = 0; i < 3; ++i) {
            if (!m_tabLabels[i]) continue;
            m_tabLabels[i]->setColor(i == tab ? ccColor3B{ 80, 210, 255 } : ccColor3B{ 125, 145, 175 });
            m_tabLabels[i]->setScale(i == tab ? 0.5f : 0.44f);
        }

        const char* hints[] = {
            "Player tools are saved automatically and work in normal play.",
            "Generate into an empty level. Decor stays above jump height.",
            "Visual helpers. Hitboxes and HUD update live during a run.",
        };
        if (m_hint) m_hint->setString(hints[tab]);
    }

    bool init() {
        if (!Popup::init(580.f, 380.f)) return false;

        auto* surface = CCLayerColor::create(ccColor4B{ 7, 16, 36, 245 }, 570.f, 370.f);
        surface->setPosition({ 5.f, 5.f });
        m_mainLayer->addChild(surface, 1);

        auto* topLine = CCLayerColor::create(ccColor4B{ 65, 205, 255, 255 }, 570.f, 3.f);
        topLine->setPosition({ 5.f, 372.f });
        m_mainLayer->addChild(topLine, 2);

        auto* title = CCLabelBMFont::create("GSTGEODE", "goldFont.fnt");
        title->setScale(0.92f);
        title->setColor({ 89, 213, 255 });
        title->setPosition({ 290.f, 342.f });
        m_mainLayer->addChild(title, 3);

        auto* subtitle = CCLabelBMFont::create("CREATOR  /  PRACTICE  /  CONTROL", "bigFont.fnt");
        subtitle->setScale(0.38f);
        subtitle->setColor({ 166, 192, 224 });
        subtitle->setPosition({ 290.f, 318.f });
        m_mainLayer->addChild(subtitle, 3);

        // Sidebar
        auto* sidebar = CCLayerColor::create(ccColor4B{ 15, 30, 56, 255 }, 120.f, 276.f);
        sidebar->setPosition({ 16.f, 24.f });
        m_mainLayer->addChild(sidebar, 2);

        // Content card
        auto* content = CCLayerColor::create(ccColor4B{ 15, 30, 56, 255 }, 418.f, 276.f);
        content->setPosition({ 146.f, 24.f });
        m_mainLayer->addChild(content, 2);

        auto* tabMenu = CCMenu::create();
        tabMenu->setPosition({ 0.f, 0.f });
        m_mainLayer->addChild(tabMenu, 6);

        const char* tabNames[] = { "PLAYER", "BUILDER", "VISUAL" };
        const float tabY[] = { 260.f, 220.f, 180.f };

        for (int i = 0; i < 3; ++i) {
            auto* label = CCLabelBMFont::create(tabNames[i], "bigFont.fnt");
            label->setScale(0.44f);
            label->setColor({ 125, 145, 175 });
            auto* item = CCMenuItemLabel::create(label, this, menu_selector(GSTControlPanel::onTab));
            item->setPosition({ 76.f, tabY[i] });
            item->setTag(i);
            tabMenu->addChild(item);
            m_tabLabels[i] = label;
        }

        m_playerPage = makePage();
        m_builderPage = makePage();
        m_visualPage = makePage();

        // -- PLAYER --------------------------------------------------------
        m_noclipLabel = addRow(m_playerPage, "NOCLIP", "OFF", { 355.f, 250.f },
            menu_selector(GSTControlPanel::onToggleNoclip));
        m_speedLabel = addRow(m_playerPage, "SPEEDHACK", "OFF", { 355.f, 216.f },
            menu_selector(GSTControlPanel::onToggleSpeed));
        m_rateLabel = addRow(m_playerPage, "SPEED RATE", "1.5x", { 355.f, 182.f },
            menu_selector(GSTControlPanel::onCycleSpeed));
        m_restartLabel = addRow(m_playerPage, "AUTO RESTART", "OFF", { 355.f, 148.f },
            menu_selector(GSTControlPanel::onToggleRestart));
        m_practiceLabel = addRow(m_playerPage, "FORCE PRACTICE", "OFF", { 355.f, 114.f },
            menu_selector(GSTControlPanel::onTogglePractice));

        addAction(m_playerPage, "RESET ALL", { 355.f, 62.f },
            menu_selector(GSTControlPanel::onResetAll), 0.62f);

        // -- BUILDER -------------------------------------------------------
        m_modeLabel = addRow(m_builderPage, "MODE", "MIX", { 250.f, 250.f },
            menu_selector(GSTControlPanel::onCycleMode), 200.f);
        m_themeLabel = addRow(m_builderPage, "THEME", "NEON", { 250.f, 216.f },
            menu_selector(GSTControlPanel::onCycleTheme), 200.f);
        m_difficultyLabel = addRow(m_builderPage, "DIFFICULTY", "NORMAL", { 250.f, 182.f },
            menu_selector(GSTControlPanel::onCycleDifficulty), 200.f);
        m_lengthLabel = addRow(m_builderPage, "LENGTH", "MEDIUM", { 250.f, 148.f },
            menu_selector(GSTControlPanel::onCycleLength), 200.f);

        m_decorLabel = addRow(m_builderPage, "DECOR", "FULL", { 460.f, 250.f },
            menu_selector(GSTControlPanel::onCycleDecor), 200.f);
        m_styleLabel = addRow(m_builderPage, "STYLE", "FLOW", { 460.f, 216.f },
            menu_selector(GSTControlPanel::onCycleStyle), 200.f);
        m_variantLabel = addRow(m_builderPage, "VARIANT", "V1", { 460.f, 182.f },
            menu_selector(GSTControlPanel::onCycleVariant), 200.f);

        addAction(m_builderPage, "GENERATE LEVEL", { 355.f, 112.f },
            menu_selector(GSTControlPanel::onGenerate), 0.72f);
        addAction(m_builderPage, "RANDOMIZE", { 355.f, 66.f },
            menu_selector(GSTControlPanel::onRandomize), 0.58f);

        // -- VISUAL --------------------------------------------------------
        m_hitboxLabel = addRow(m_visualPage, "HITBOXES", "OFF", { 355.f, 250.f },
            menu_selector(GSTControlPanel::onToggleHitboxes));
        m_hudLabel = addRow(m_visualPage, "HUD", "ON", { 355.f, 216.f },
            menu_selector(GSTControlPanel::onToggleHud));
        m_fpsLabel = addRow(m_visualPage, "SHOW FPS", "ON", { 355.f, 182.f },
            menu_selector(GSTControlPanel::onToggleFps));
        m_percentLabel = addRow(m_visualPage, "SHOW PERCENT", "ON", { 355.f, 148.f },
            menu_selector(GSTControlPanel::onTogglePercent));

        m_hint = CCLabelBMFont::create("", "bigFont.fnt");
        m_hint->setScale(0.3f);
        m_hint->setColor({ 145, 168, 198 });
        m_hint->setPosition({ 355.f, 40.f });
        m_mainLayer->addChild(m_hint, 3);

        setTab(0);
        refreshLabels();
        return true;
    }

public:
    static GSTControlPanel* create() {
        auto* panel = new GSTControlPanel();
        if (panel && panel->init()) {
            panel->autorelease();
            return panel;
        }
        delete panel;
        return nullptr;
    }

    void onTab(CCObject* sender) {
        setTab(sender ? sender->getTag() : 0);
    }

    void onToggleNoclip(CCObject*) {
        gst::noclipEnabled = !gst::noclipEnabled;
        gst::saveSettings();
        refreshLabels();
    }

    void onToggleSpeed(CCObject*) {
        gst::speedEnabled = !gst::speedEnabled;
        gst::saveSettings();
        refreshLabels();
    }

    void onCycleSpeed(CCObject*) {
        gst::speedIndex = (gst::speedIndex + 1) % 5;
        gst::saveSettings();
        refreshLabels();
    }

    void onToggleRestart(CCObject*) {
        gst::autoRestart = !gst::autoRestart;
        gst::saveSettings();
        refreshLabels();
    }

    void onTogglePractice(CCObject*) {
        gst::forcePractice = !gst::forcePractice;
        gst::saveSettings();
        refreshLabels();
    }

    void onCycleMode(CCObject*) {
        gst::modeIndex = (gst::modeIndex + 1) % 9;
        gst::saveSettings();
        refreshLabels();
    }

    void onCycleTheme(CCObject*) {
        gst::themeIndex = (gst::themeIndex + 1) % gst::themeCount;
        gst::saveSettings();
        refreshLabels();
    }

    void onCycleDifficulty(CCObject*) {
        gst::difficultyIndex = (gst::difficultyIndex + 1) % 4;
        gst::saveSettings();
        refreshLabels();
    }

    void onCycleLength(CCObject*) {
        gst::lengthIndex = (gst::lengthIndex + 1) % 3;
        gst::saveSettings();
        refreshLabels();
    }

    void onCycleDecor(CCObject*) {
        gst::decorIndex = (gst::decorIndex + 1) % 3;
        gst::saveSettings();
        refreshLabels();
    }

    void onCycleStyle(CCObject*) {
        gst::styleIndex = (gst::styleIndex + 1) % 3;
        gst::saveSettings();
        refreshLabels();
    }

    void onCycleVariant(CCObject*) {
        gst::variantIndex = (gst::variantIndex + 1) % 4;
        gst::saveSettings();
        refreshLabels();
    }

    void onRandomize(CCObject*) {
        gst::themeIndex = std::rand() % gst::themeCount;
        gst::styleIndex = std::rand() % 3;
        gst::variantIndex = std::rand() % 4;
        gst::difficultyIndex = std::rand() % 4;
        gst::saveSettings();
        refreshLabels();
    }

    void onToggleHitboxes(CCObject*) {
        gst::hitboxes = !gst::hitboxes;
        gst::saveSettings();
        refreshLabels();
    }

    void onToggleHud(CCObject*) {
        gst::hudEnabled = !gst::hudEnabled;
        gst::saveSettings();
        refreshLabels();
    }

    void onToggleFps(CCObject*) {
        gst::showFps = !gst::showFps;
        gst::saveSettings();
        refreshLabels();
    }

    void onTogglePercent(CCObject*) {
        gst::showPercent = !gst::showPercent;
        gst::saveSettings();
        refreshLabels();
    }

    void onResetAll(CCObject*) {
        gst::noclipEnabled = false;
        gst::speedEnabled = false;
        gst::autoRestart = false;
        gst::forcePractice = false;
        gst::hitboxes = false;
        gst::hudEnabled = true;
        gst::showFps = true;
        gst::showPercent = true;
        gst::speedIndex = 2;
        gst::themeIndex = 0;
        gst::difficultyIndex = 1;
        gst::lengthIndex = 1;
        gst::decorIndex = 2;
        gst::modeIndex = 0;
        gst::styleIndex = 0;
        gst::variantIndex = 0;
        gst::saveSettings();
        refreshLabels();
    }

    void onGenerate(CCObject*) {
        gst::buildLayout();
        this->onClose(nullptr);
    }
};

namespace gst {
    void openPanel() {
        loadSettings();
        log::info("GSTgeode panel opened");
        if (auto* panel = GSTControlPanel::create()) panel->show();
    }
}

$on_mod(Loaded) {
    gst::loadSettings();
}

// -------------------------------------------------------------------------
// Entry points
// -------------------------------------------------------------------------

class $modify(GSTMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto* menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });

        auto* item = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("GST", "goldFont.fnt", "GJ_button_01.png", 0.72f),
            this,
            menu_selector(GSTMenuLayer::onGST)
        );
        item->setPosition({ 42.f, winSize.height - 96.f });
        menu->addChild(item);
        this->addChild(menu, 20);
        return true;
    }

    void onGST(CCObject*) {
        gst::openPanel();
    }
};

class $modify(GSTEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto* menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });

        auto* item = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("GST", "goldFont.fnt", "GJ_button_01.png", 0.7f),
            this,
            menu_selector(GSTEditorUI::onGST)
        );
        item->setPosition({ winSize.width - 44.f, winSize.height - 96.f });
        menu->addChild(item);
        this->addChild(menu, 9999);
        return true;
    }

    void onGST(CCObject*) {
        gst::openPanel();
    }
};

class $modify(GSTPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto* menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });

        auto* item = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("GST TOOLS", "goldFont.fnt", "GJ_button_01.png", 0.62f),
            this,
            menu_selector(GSTPauseLayer::onGST)
        );
        item->setPosition({ winSize.width / 2.f, 78.f });
        menu->addChild(item);
        this->addChild(menu, 9999);
    }

    void onGST(CCObject*) {
        gst::openPanel();
    }
};

// Noclip is a death bypass: the player-destruction call is suppressed.
class $modify(GSTPlayLayer, PlayLayer) {
    struct Fields {
        CCLabelBMFont* hud = nullptr;
        CCDrawNode* hitboxNode = nullptr;
        float fps = 0.f;
        float fpsAccum = 0.f;
        int fpsFrames = 0;
    };

    bool init(GJGameLevel* level, bool dontCreateObjects) {
        if (!PlayLayer::init(level, dontCreateObjects)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        m_fields->hud = CCLabelBMFont::create("GST", "bigFont.fnt");
        if (m_fields->hud) {
            m_fields->hud->setScale(0.32f);
            m_fields->hud->setAnchorPoint({ 0.f, 1.f });
            m_fields->hud->setPosition({ 10.f, winSize.height - 8.f });
            m_fields->hud->setZOrder(999);
            this->addChild(m_fields->hud);
        }

        if (this->m_objectLayer) {
            m_fields->hitboxNode = CCDrawNode::create();
            if (m_fields->hitboxNode) {
                m_fields->hitboxNode->setZOrder(999);
                this->m_objectLayer->addChild(m_fields->hitboxNode);
            }
        }

        return true;
    }

    void startGame() {
        PlayLayer::startGame();
        if (gst::forcePractice && !this->m_isPracticeMode) {
            this->unschedule(schedule_selector(GSTPlayLayer::applyPractice));
            this->scheduleOnce(schedule_selector(GSTPlayLayer::applyPractice), 0.1f);
        }
    }

    void applyPractice(float) {
        if (gst::forcePractice && !this->m_isPracticeMode) {
            this->togglePracticeMode(true);
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (gst::noclipEnabled) return;

        PlayLayer::destroyPlayer(player, object);

        if (gst::autoRestart && !this->m_isPracticeMode) {
            this->unschedule(schedule_selector(GSTPlayLayer::restartAfterDeath));
            this->scheduleOnce(schedule_selector(GSTPlayLayer::restartAfterDeath), 0.5f);
        }
    }

    void restartAfterDeath(float) {
        if (!gst::autoRestart || this->m_isPaused) return;
        this->resetLevel();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (m_fields->hud) {
            m_fields->fpsAccum += dt;
            ++m_fields->fpsFrames;
            if (m_fields->fpsAccum >= 0.25f) {
                m_fields->fps = static_cast<float>(m_fields->fpsFrames) / m_fields->fpsAccum;
                m_fields->fpsAccum = 0.f;
                m_fields->fpsFrames = 0;
            }

            m_fields->hud->setVisible(gst::hudEnabled);
            if (gst::hudEnabled) {
                std::string text;
                if (gst::showPercent) {
                    text += fmt::format("{}%  ", this->getCurrentPercentInt());
                }
                if (gst::showFps) {
                    text += fmt::format("{:.0f} FPS  ", m_fields->fps);
                }
                if (gst::noclipEnabled) text += "NOCLIP  ";
                if (gst::speedEnabled) {
                    text += fmt::format("{}x", gst::speeds[gst::speedIndex]);
                }
                if (text.empty()) text = "GST";
                m_fields->hud->setString(text.c_str());
            }
        }

        if (!m_fields->hitboxNode && this->m_objectLayer) {
            m_fields->hitboxNode = CCDrawNode::create();
            if (m_fields->hitboxNode) {
                m_fields->hitboxNode->setZOrder(999);
                this->m_objectLayer->addChild(m_fields->hitboxNode);
            }
        }

        if (m_fields->hitboxNode) {
            m_fields->hitboxNode->setVisible(gst::hitboxes);
            if (gst::hitboxes) {
                m_fields->hitboxNode->clear();

                auto draw = [&](PlayerObject* player, ccColor4F color) {
                    if (!player || player->m_isDead) return;
                    auto const& rect = player->getObjectRect();
                    ccColor4F fill = { color.r, color.g, color.b, 0.18f };
                    m_fields->hitboxNode->drawRect(rect, fill, 1.f, color);
                };

                draw(this->m_player1, { 1.f, 0.25f, 0.25f, 1.f });
                draw(this->m_player2, { 0.25f, 0.7f, 1.f, 1.f });
            }
        }
    }
};

// Speedhack multiplies delta time only for the active PlayLayer.
class $modify(GSTBaseGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        auto* playLayer = PlayLayer::get();
        auto* activeGameLayer = static_cast<GJBaseGameLayer*>(playLayer);
        if (playLayer && activeGameLayer == this && gst::speedEnabled) {
            GJBaseGameLayer::update(dt * gst::speeds[gst::speedIndex]);
            return;
        }
        GJBaseGameLayer::update(dt);
    }
};
