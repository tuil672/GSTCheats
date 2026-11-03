#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

namespace gst {
    // ---------------------------------------------------------------------
    // Persistent runtime state
    // ---------------------------------------------------------------------
    bool noclipEnabled = false;
    bool speedEnabled = false;
    int speedIndex = 2;
    int themeIndex = 0;
    int difficultyIndex = 1;
    int lengthIndex = 1;
    int decorIndex = 2;

    constexpr const char* difficulties[] = { "EASY", "NORMAL", "HARD", "DEMON" };
    constexpr const char* lengths[] = { "SHORT", "MEDIUM", "LONG" };
    constexpr const char* decorLevels[] = { "OFF", "LIGHT", "FULL" };
    constexpr const char* speedNames[] = { "1.0x", "1.25x", "1.5x", "2.0x", "3.0x" };
    constexpr float speeds[] = { 1.0f, 1.25f, 1.5f, 2.0f, 3.0f };

    struct Theme {
        const char* name;
        ccColor3B primary;
        ccColor3B accent;
        ccColor3B detail;
    };

    // Palettes are chosen so the generated layout reads well against the
    // default (dark blue) editor background.
    Theme themes[] = {
        { "NEON",   { 61, 203, 255 }, { 255,  74, 185 }, { 112, 115, 255 } },
        { "SUNSET", { 255, 126, 91 }, { 255,  75, 151 }, { 185,  91, 184 } },
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

    void loadSettings() {
        auto* mod = Mod::get();
        if (!mod) return;

        noclipEnabled = mod->getSavedValue<bool>("noclip", false);
        speedEnabled = mod->getSavedValue<bool>("speedhack", false);
        speedIndex = clampIndex(mod->getSavedValue<int>("speedIndex", 2), 5);
        themeIndex = clampIndex(mod->getSavedValue<int>("themeIndex", 0), themeCount);
        difficultyIndex = clampIndex(mod->getSavedValue<int>("difficultyIndex", 1), 4);
        lengthIndex = clampIndex(mod->getSavedValue<int>("lengthIndex", 1), 3);
        decorIndex = clampIndex(mod->getSavedValue<int>("decorIndex", 2), 3);
    }

    void saveSettings() {
        auto* mod = Mod::get();
        if (!mod) return;

        mod->setSavedValue<bool>("noclip", noclipEnabled);
        mod->setSavedValue<bool>("speedhack", speedEnabled);
        mod->setSavedValue<int>("speedIndex", speedIndex);
        mod->setSavedValue<int>("themeIndex", themeIndex);
        mod->setSavedValue<int>("difficultyIndex", difficultyIndex);
        mod->setSavedValue<int>("lengthIndex", lengthIndex);
        mod->setSavedValue<int>("decorIndex", decorIndex);
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
    //  * The top of the ground is at y = 90, so an object resting on the
    //    ground has its centre at y = 105.
    //  * Anything at y >= 255 is above the player's maximum jump height and
    //    is therefore safe to use as pure decoration.
    // ---------------------------------------------------------------------
    bool buildStarterLayout() {
        auto* editor = LevelEditorLayer::get();
        if (!editor || !editor->m_editorUI) {
            notify("Open a level in the editor first, then press GENERATE LEVEL.");
            return false;
        }

        auto* ui = editor->m_editorUI;
        const Theme& theme = themes[clampIndex(themeIndex, themeCount)];

        const int segmentsByLength[] = { 20, 36, 56 };
        const int spikeEveryByDifficulty[] = { 7, 5, 4, 3 };
        const int blockEveryByDifficulty[] = { 11, 9, 7, 5 };

        const int segments = segmentsByLength[clampIndex(lengthIndex, 3)];
        const int spikeEvery = spikeEveryByDifficulty[clampIndex(difficultyIndex, 4)];
        const int blockEvery = blockEveryByDifficulty[clampIndex(difficultyIndex, 4)];

        const float grid = 30.f;
        const float startX = 180.f;
        const float groundY = 105.f;
        const float decorY = 255.f;
        const float decorStep = 30.f;

        int placed = 0;

        auto add = [&](int objectID, CCPoint position, ccColor3B color,
                       float scale = 1.f, float rotation = 0.f) {
            if (auto* object = ui->createObject(objectID, position)) {
                object->setObjectColor(color);
                if (scale != 1.f) object->setScale(scale);
                if (rotation != 0.f) object->setRotation(rotation);
                ++placed;
            }
        };

        // -- Gameplay lane -------------------------------------------------
        // The player runs on the real ground, so obstacles are placed with
        // their centres at groundY. Blocks are one cell high and jumpable.
        for (int i = 3; i < segments; ++i) {
            const float x = startX + static_cast<float>(i) * grid;

            if ((i % spikeEvery) == 0) {
                add(8, { x, groundY }, theme.accent);

                // Harder difficulties occasionally require a double jump.
                if (difficultyIndex >= 2 && (i % (spikeEvery * 2)) == 0) {
                    add(8, { x + grid, groundY }, theme.accent);
                }
            } else if ((i % blockEvery) == 0) {
                add(1, { x, groundY }, theme.primary);

                if (difficultyIndex >= 3) {
                    add(1, { x, groundY + grid }, theme.primary);
                }
            }
        }

        // -- Decoration ----------------------------------------------------
        if (decorIndex > 0) {
            const int diamondStep = (decorIndex >= 2) ? 3 : 6;

            for (int i = 1; i < segments; i += diamondStep) {
                const float x = startX + static_cast<float>(i) * grid;

                // Small floating diamonds.
                add(1, { x, decorY }, theme.detail, 0.45f, 45.f);

                if ((i % (diamondStep * 2)) == 0) {
                    add(1, { x, decorY + decorStep * 2.f }, theme.accent, 0.7f, 45.f);
                }
            }

            // Full decoration adds arches built from standard blocks.
            if (decorIndex >= 2) {
                for (int i = 5; i < segments; i += 12) {
                    const float x = startX + static_cast<float>(i) * grid;

                    for (int pillar = 0; pillar < 2; ++pillar) {
                        const float px = x + static_cast<float>(pillar) * grid * 4.f;
                        for (int row = 0; row < 3; ++row) {
                            add(1, { px, decorY + static_cast<float>(row) * grid },
                                (row % 2 == 0) ? theme.primary : theme.detail);
                        }
                    }

                    for (int beam = 0; beam < 5; ++beam) {
                        add(1, { x + static_cast<float>(beam) * grid, decorY + grid * 3.f },
                            (beam % 2 == 0) ? theme.detail : theme.accent);
                    }
                }
            }
        }

        const std::string message = fmt::format(
            "GSTgeode placed {} objects ({} / {} / {} decor). "
            "Use Undo if you want to remove them.",
            placed,
            difficulties[clampIndex(difficultyIndex, 4)],
            lengths[clampIndex(lengthIndex, 3)],
            decorLevels[clampIndex(decorIndex, 3)]
        );
        notify(message.c_str());
        return true;
    }
}

// -------------------------------------------------------------------------
// Control panel
// -------------------------------------------------------------------------
class GSTControlPanel : public Popup {
protected:
    CCLabelBMFont* m_noclipLabel = nullptr;
    CCLabelBMFont* m_speedLabel = nullptr;
    CCLabelBMFont* m_speedRateLabel = nullptr;
    CCLabelBMFont* m_themeLabel = nullptr;
    CCLabelBMFont* m_difficultyLabel = nullptr;
    CCLabelBMFont* m_lengthLabel = nullptr;
    CCLabelBMFont* m_decorLabel = nullptr;

    CCLabelBMFont* addMenuLabel(
        CCMenu* menu,
        const char* text,
        CCPoint position,
        SEL_MenuHandler callback,
        ccColor3B color = { 235, 244, 255 },
        float scale = 0.46f
    ) {
        auto* label = CCLabelBMFont::create(text, "bigFont.fnt");
        if (!label) return nullptr;
        label->setColor(color);
        label->setScale(scale);
        auto* item = CCMenuItemLabel::create(label, this, callback);
        item->setPosition(position);
        menu->addChild(item);
        return label;
    }

    void refreshLabels() {
        if (m_noclipLabel) {
            m_noclipLabel->setString(gst::noclipEnabled ? "NOCLIP: ON" : "NOCLIP: OFF");
            m_noclipLabel->setColor(gst::noclipEnabled ? ccColor3B{ 92, 235, 150 } : ccColor3B{ 235, 244, 255 });
        }
        if (m_speedLabel) {
            m_speedLabel->setString(gst::speedEnabled ? "SPEEDHACK: ON" : "SPEEDHACK: OFF");
            m_speedLabel->setColor(gst::speedEnabled ? ccColor3B{ 92, 235, 150 } : ccColor3B{ 235, 244, 255 });
        }
        if (m_speedRateLabel) {
            m_speedRateLabel->setString((std::string("RATE: ") + gst::speedNames[gst::speedIndex]).c_str());
        }
        if (m_themeLabel) {
            m_themeLabel->setString((std::string("THEME: ") + gst::themes[gst::themeIndex].name).c_str());
        }
        if (m_difficultyLabel) {
            m_difficultyLabel->setString((std::string("DIFFICULTY: ") + gst::difficulties[gst::difficultyIndex]).c_str());
        }
        if (m_lengthLabel) {
            m_lengthLabel->setString((std::string("LENGTH: ") + gst::lengths[gst::lengthIndex]).c_str());
        }
        if (m_decorLabel) {
            m_decorLabel->setString((std::string("DECOR: ") + gst::decorLevels[gst::decorIndex]).c_str());
        }
    }

    bool init() {
        if (!Popup::init(460.f, 340.f)) return false;

        // Deep navy surface with cyan / magenta accent cards.
        auto* surface = CCLayerColor::create(ccColor4B{ 7, 16, 36, 242 }, 450.f, 330.f);
        surface->setPosition({ 5.f, 5.f });
        this->addChild(surface, -1);

        auto* topLine = CCLayerColor::create(ccColor4B{ 65, 205, 255, 255 }, 450.f, 3.f);
        topLine->setPosition({ 5.f, 332.f });
        this->addChild(topLine, 1);

        auto* leftCard = CCLayerColor::create(ccColor4B{ 17, 34, 62, 255 }, 204.f, 148.f);
        leftCard->setPosition({ 18.f, 112.f });
        this->addChild(leftCard, 0);
        auto* rightCard = CCLayerColor::create(ccColor4B{ 17, 34, 62, 255 }, 204.f, 178.f);
        rightCard->setPosition({ 238.f, 112.f });
        this->addChild(rightCard, 0);

        auto* title = CCLabelBMFont::create("GSTGEODE", "goldFont.fnt");
        title->setScale(0.86f);
        title->setColor({ 89, 213, 255 });
        title->setPosition({ 230.f, 306.f });
        this->addChild(title, 2);

        auto* subtitle = CCLabelBMFont::create("CREATOR  /  PRACTICE  /  CONTROL", "bigFont.fnt");
        subtitle->setScale(0.38f);
        subtitle->setColor({ 166, 192, 224 });
        subtitle->setPosition({ 230.f, 282.f });
        this->addChild(subtitle, 2);

        auto* playerTitle = CCLabelBMFont::create("PLAYER TOOLS", "bigFont.fnt");
        playerTitle->setScale(0.46f);
        playerTitle->setColor({ 74, 202, 255 });
        playerTitle->setPosition({ 120.f, 242.f });
        this->addChild(playerTitle, 2);

        auto* builderTitle = CCLabelBMFont::create("LEVEL BUILDER", "bigFont.fnt");
        builderTitle->setScale(0.46f);
        builderTitle->setColor({ 255, 112, 194 });
        builderTitle->setPosition({ 340.f, 274.f });
        this->addChild(builderTitle, 2);

        auto* menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        this->addChild(menu, 5);

        m_noclipLabel = addMenuLabel(menu, "NOCLIP: OFF", { 120.f, 212.f }, menu_selector(GSTControlPanel::onToggleNoclip));
        m_speedLabel = addMenuLabel(menu, "SPEEDHACK: OFF", { 120.f, 182.f }, menu_selector(GSTControlPanel::onToggleSpeed));
        m_speedRateLabel = addMenuLabel(menu, "RATE: 1.5x", { 120.f, 152.f }, menu_selector(GSTControlPanel::onCycleSpeed));

        m_themeLabel = addMenuLabel(menu, "THEME: NEON", { 340.f, 242.f }, menu_selector(GSTControlPanel::onCycleTheme));
        m_difficultyLabel = addMenuLabel(menu, "DIFFICULTY: NORMAL", { 340.f, 212.f }, menu_selector(GSTControlPanel::onCycleDifficulty));
        m_lengthLabel = addMenuLabel(menu, "LENGTH: MEDIUM", { 340.f, 182.f }, menu_selector(GSTControlPanel::onCycleLength));
        m_decorLabel = addMenuLabel(menu, "DECOR: FULL", { 340.f, 152.f }, menu_selector(GSTControlPanel::onCycleDecor));

        auto* buildLabel = addMenuLabel(
            menu, "GENERATE LEVEL", { 230.f, 76.f },
            menu_selector(GSTControlPanel::onGenerate),
            { 255, 218, 106 }, 0.68f
        );
        if (buildLabel) buildLabel->setColor({ 255, 218, 106 });

        auto* hint = CCLabelBMFont::create("Use in an empty editor level; generated objects can be undone.", "bigFont.fnt");
        hint->setScale(0.31f);
        hint->setColor({ 145, 168, 198 });
        hint->setPosition({ 230.f, 42.f });
        this->addChild(hint, 2);

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

    void onGenerate(CCObject*) {
        gst::buildStarterLayout();
    }
};

namespace gst {
    void openPanel() {
        loadSettings();
        if (auto* panel = GSTControlPanel::create()) panel->show();
    }
}

// Load persisted toggles as soon as the mod is loaded.
$on_mod(Loaded) {
    gst::loadSettings();
}

// -------------------------------------------------------------------------
// Entry points
// -------------------------------------------------------------------------

// Main-menu shortcut.
class $modify(GSTMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (auto* bottomMenu = this->getChildByID("bottom-menu")) {
            auto* label = CCLabelBMFont::create("GST", "goldFont.fnt");
            label->setScale(0.56f);
            label->setColor({ 80, 210, 255 });
            auto* item = CCMenuItemLabel::create(label, this, menu_selector(GSTMenuLayer::onGST));
            bottomMenu->addChild(item);
            bottomMenu->updateLayout();
        }
        return true;
    }

    void onGST(CCObject*) {
        gst::openPanel();
    }
};

// Editor shortcut. The builder button is placed in the upper-right corner.
class $modify(GSTEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto* menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        auto* label = CCLabelBMFont::create("GST", "goldFont.fnt");
        label->setScale(0.58f);
        label->setColor({ 76, 213, 255 });
        auto* item = CCMenuItemLabel::create(label, this, menu_selector(GSTEditorUI::onGST));
        item->setPosition({ winSize.width - 42.f, winSize.height - 35.f });
        menu->addChild(item);
        this->addChild(menu, 9999);
        return true;
    }

    void onGST(CCObject*) {
        gst::openPanel();
    }
};

// Pause-menu shortcut so the player tools are reachable during a run.
class $modify(GSTPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto* menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        auto* label = CCLabelBMFont::create("GST TOOLS", "goldFont.fnt");
        label->setScale(0.58f);
        label->setColor({ 75, 213, 255 });
        auto* item = CCMenuItemLabel::create(label, this, menu_selector(GSTPauseLayer::onGST));
        item->setPosition({ winSize.width / 2.f, 76.f });
        menu->addChild(item);
        this->addChild(menu, 9999);
    }

    void onGST(CCObject*) {
        gst::openPanel();
    }
};

// Noclip is implemented as a death bypass (the player is not destroyed).
// It is not a full collision / ghost-mode rewrite.
class $modify(GSTPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (gst::noclipEnabled) return;
        PlayLayer::destroyPlayer(player, object);
    }
};

// Multiply delta time only for the active PlayLayer. The explicit upcast
// avoids comparing PlayLayer* with GJBaseGameLayer* (a compile-time error).
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
