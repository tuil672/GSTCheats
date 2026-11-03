#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

namespace gst {
    // Runtime state is intentionally kept in memory for this first build.
    bool noclipEnabled = false;
    bool speedEnabled = false;
    int speedIndex = 2;
    int themeIndex = 0;
    int difficultyIndex = 1;
    int lengthIndex = 1;

    constexpr const char* themes[] = { "NEON", "SUNSET", "FOREST", "GOLD" };
    constexpr const char* difficulties[] = { "EASY", "NORMAL", "HARD", "DEMON" };
    constexpr const char* lengths[] = { "SHORT", "MEDIUM", "LONG" };
    constexpr float speeds[] = { 1.0f, 1.25f, 1.5f, 2.0f, 3.0f };
    constexpr const char* speedNames[] = { "1.0x", "1.25x", "1.5x", "2.0x", "3.0x" };

    ccColor3B primaryColor() {
        switch (themeIndex) {
            case 1: return {255, 126, 91};
            case 2: return {92, 220, 147};
            case 3: return {255, 204, 76};
            default: return {61, 203, 255};
        }
    }

    ccColor3B accentColor() {
        switch (themeIndex) {
            case 1: return {255, 75, 151};
            case 2: return {49, 165, 142};
            case 3: return {255, 145, 58};
            default: return {255, 74, 185};
        }
    }

    ccColor3B detailColor() {
        switch (themeIndex) {
            case 1: return {185, 91, 184};
            case 2: return {175, 242, 130};
            case 3: return {255, 238, 151};
            default: return {112, 115, 255};
        }
    }

    void notify(const char* message) {
        FLAlertLayer::create("GSTgeode", message, "OK")->show();
    }

    void openPanel();

    bool buildStarterLayout() {
        auto* editor = LevelEditorLayer::get();
        if (!editor || !editor->m_editorUI) {
            notify("Open a level in the editor first, then press GENERATE LEVEL.");
            return false;
        }

        auto* ui = editor->m_editorUI;
        const int segmentCounts[] = { 16, 28, 40 };
        const int spikeIntervals[] = { 8, 6, 4, 3 };
        const int segmentCount = segmentCounts[lengthIndex];
        const int spikeEvery = spikeIntervals[difficultyIndex];
        const float startX = 180.f;
        const float stepX = 60.f;
        const float floorY = 105.f;
        int placed = 0;

        auto add = [&](int objectID, CCPoint position, ccColor3B color) {
            if (auto* object = ui->createObject(objectID, position)) {
                object->setObjectColor(color);
                ++placed;
            }
        };

        // A simple runway made of standard editor objects. The player can
        // recolor/redecorate it and continue building manually.
        for (int i = 0; i < segmentCount; ++i) {
            const float x = startX + static_cast<float>(i) * stepX;
            add(1, {x, floorY}, primaryColor());

            // Raised accent blocks create a repeating visual rhythm.
            if (i % 2 == 0) {
                add(1, {x, 255.f}, detailColor());
            }
            if (i % 6 == 0) {
                add(1, {x + 30.f, 285.f}, accentColor());
            }

            // Difficulty changes obstacle frequency, not object IDs.
            if (i > 1 && (i % spikeEvery) == (spikeEvery - 1)) {
                add(8, {x + 15.f, floorY + 30.f}, accentColor());
            }

            // Short pillars act as simple decorative accents.
            if (i % 8 == 4) {
                add(1, {x + 15.f, 165.f}, detailColor());
                add(1, {x + 15.f, 195.f}, detailColor());
            }
        }

        const std::string message = fmt::format(
            "GSTgeode placed {} objects. Use Undo if you want to remove the generated layout.",
            placed
        );
        notify(message.c_str());
        return true;
    }
}

class GSTControlPanel : public Popup {
protected:
    CCLabelBMFont* m_noclipLabel = nullptr;
    CCLabelBMFont* m_speedLabel = nullptr;
    CCLabelBMFont* m_speedRateLabel = nullptr;
    CCLabelBMFont* m_themeLabel = nullptr;
    CCLabelBMFont* m_difficultyLabel = nullptr;
    CCLabelBMFont* m_lengthLabel = nullptr;

    CCLabelBMFont* addMenuLabel(
        CCMenu* menu,
        const char* text,
        CCPoint position,
        SEL_MenuHandler callback,
        ccColor3B color = {235, 244, 255},
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
            m_noclipLabel->setColor(gst::noclipEnabled ? ccColor3B{92, 235, 150} : ccColor3B{235, 244, 255});
        }
        if (m_speedLabel) {
            m_speedLabel->setString(gst::speedEnabled ? "SPEEDHACK: ON" : "SPEEDHACK: OFF");
            m_speedLabel->setColor(gst::speedEnabled ? ccColor3B{92, 235, 150} : ccColor3B{235, 244, 255});
        }
        if (m_speedRateLabel) {
            m_speedRateLabel->setString((std::string("RATE: ") + gst::speedNames[gst::speedIndex]).c_str());
        }
        if (m_themeLabel) {
            m_themeLabel->setString((std::string("THEME: ") + gst::themes[gst::themeIndex]).c_str());
        }
        if (m_difficultyLabel) {
            m_difficultyLabel->setString((std::string("DIFFICULTY: ") + gst::difficulties[gst::difficultyIndex]).c_str());
        }
        if (m_lengthLabel) {
            m_lengthLabel->setString((std::string("LENGTH: ") + gst::lengths[gst::lengthIndex]).c_str());
        }
    }

    bool init() {
        if (!Popup::init(440.f, 320.f)) return false;

        // Deep navy surface with cyan / magenta accent cards.
        auto* surface = CCLayerColor::create(ccColor4B{7, 16, 36, 242}, 430.f, 310.f);
        surface->setPosition({5.f, 5.f});
        this->addChild(surface, -1);

        auto* topLine = CCLayerColor::create(ccColor4B{65, 205, 255, 255}, 430.f, 3.f);
        topLine->setPosition({5.f, 312.f});
        this->addChild(topLine, 1);

        auto* leftCard = CCLayerColor::create(ccColor4B{17, 34, 62, 255}, 194.f, 124.f);
        leftCard->setPosition({18.f, 118.f});
        this->addChild(leftCard, 0);
        auto* rightCard = CCLayerColor::create(ccColor4B{17, 34, 62, 255}, 194.f, 124.f);
        rightCard->setPosition({228.f, 118.f});
        this->addChild(rightCard, 0);

        auto* title = CCLabelBMFont::create("GSTGEODE", "goldFont.fnt");
        title->setScale(0.86f);
        title->setColor({89, 213, 255});
        title->setPosition({220.f, 285.f});
        this->addChild(title, 2);

        auto* subtitle = CCLabelBMFont::create("CREATOR  /  PRACTICE  /  CONTROL", "bigFont.fnt");
        subtitle->setScale(0.38f);
        subtitle->setColor({166, 192, 224});
        subtitle->setPosition({220.f, 260.f});
        this->addChild(subtitle, 2);

        auto* playerTitle = CCLabelBMFont::create("PLAYER TOOLS", "bigFont.fnt");
        playerTitle->setScale(0.48f);
        playerTitle->setColor({74, 202, 255});
        playerTitle->setPosition({115.f, 224.f});
        this->addChild(playerTitle, 2);

        auto* builderTitle = CCLabelBMFont::create("LEVEL BUILDER", "bigFont.fnt");
        builderTitle->setScale(0.48f);
        builderTitle->setColor({255, 112, 194});
        builderTitle->setPosition({325.f, 224.f});
        this->addChild(builderTitle, 2);

        auto* menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        this->addChild(menu, 5);

        m_noclipLabel = addMenuLabel(menu, "NOCLIP: OFF", {115.f, 192.f}, menu_selector(GSTControlPanel::onToggleNoclip));
        m_speedLabel = addMenuLabel(menu, "SPEEDHACK: OFF", {115.f, 162.f}, menu_selector(GSTControlPanel::onToggleSpeed));
        m_speedRateLabel = addMenuLabel(menu, "RATE: 1.5x", {115.f, 134.f}, menu_selector(GSTControlPanel::onCycleSpeed));

        m_themeLabel = addMenuLabel(menu, "THEME: NEON", {325.f, 192.f}, menu_selector(GSTControlPanel::onCycleTheme));
        m_difficultyLabel = addMenuLabel(menu, "DIFFICULTY: NORMAL", {325.f, 162.f}, menu_selector(GSTControlPanel::onCycleDifficulty));
        m_lengthLabel = addMenuLabel(menu, "LENGTH: MEDIUM", {325.f, 134.f}, menu_selector(GSTControlPanel::onCycleLength));

        auto* buildLabel = addMenuLabel(
            menu, "GENERATE LEVEL", {220.f, 78.f},
            menu_selector(GSTControlPanel::onGenerate),
            {255, 218, 106}, 0.68f
        );
        if (buildLabel) buildLabel->setColor({255, 218, 106});

        auto* hint = CCLabelBMFont::create("Use in an empty editor level; generated objects can be undone.", "bigFont.fnt");
        hint->setScale(0.31f);
        hint->setColor({145, 168, 198});
        hint->setPosition({220.f, 43.f});
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
        refreshLabels();
    }

    void onToggleSpeed(CCObject*) {
        gst::speedEnabled = !gst::speedEnabled;
        refreshLabels();
    }

    void onCycleSpeed(CCObject*) {
        gst::speedIndex = (gst::speedIndex + 1) % 5;
        refreshLabels();
    }

    void onCycleTheme(CCObject*) {
        gst::themeIndex = (gst::themeIndex + 1) % 4;
        refreshLabels();
    }

    void onCycleDifficulty(CCObject*) {
        gst::difficultyIndex = (gst::difficultyIndex + 1) % 4;
        refreshLabels();
    }

    void onCycleLength(CCObject*) {
        gst::lengthIndex = (gst::lengthIndex + 1) % 3;
        refreshLabels();
    }

    void onGenerate(CCObject*) {
        gst::buildStarterLayout();
    }
};

namespace gst {
    void openPanel() {
        if (auto* panel = GSTControlPanel::create()) panel->show();
    }
}

// Main-menu shortcut.
class $modify(GSTMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (auto* bottomMenu = this->getChildByID("bottom-menu")) {
            auto* label = CCLabelBMFont::create("GST", "goldFont.fnt");
            label->setScale(0.56f);
            label->setColor({80, 210, 255});
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
        menu->setPosition({0.f, 0.f});
        auto* label = CCLabelBMFont::create("GST", "goldFont.fnt");
        label->setScale(0.58f);
        label->setColor({76, 213, 255});
        auto* item = CCMenuItemLabel::create(label, this, menu_selector(GSTEditorUI::onGST));
        item->setPosition({winSize.width - 42.f, winSize.height - 35.f});
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
        menu->setPosition({0.f, 0.f});
        auto* label = CCLabelBMFont::create("GST TOOLS", "goldFont.fnt");
        label->setScale(0.58f);
        label->setColor({75, 213, 255});
        auto* item = CCMenuItemLabel::create(label, this, menu_selector(GSTPauseLayer::onGST));
        item->setPosition({winSize.width / 2.f, 76.f});
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

// Multiply the game-layer delta time only while PlayLayer is active.
class $modify(GSTBaseGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        if (PlayLayer::get() == this && gst::speedEnabled) {
            GJBaseGameLayer::update(dt * gst::speeds[gst::speedIndex]);
            return;
        }
        GJBaseGameLayer::update(dt);
    }
};
