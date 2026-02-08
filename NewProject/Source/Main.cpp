/*
  ==============================================================================
    OrbitAudio: menu bar app with Control Center-style panel.
  ==============================================================================
*/

#include <JuceHeader.h>
#include "MainComponent.h"
#if JUCE_MAC
#include "MacOSStyleLookAndFeel.h"
#endif

#if JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX || JUCE_BSD

//==============================================================================
static juce::Image createMenuBarIcon (bool asTemplate)
{
    const int size = 22;
    juce::Image img (juce::Image::ARGB, size, size, true);
    juce::Graphics g (img);
    g.setColour (asTemplate ? juce::Colours::black : juce::Colour (0xff4a9eff));
    const float cx = size * 0.5f;
    const float cy = size * 0.5f;
    g.fillEllipse (cx - 6, cy - 6, 12, 12);
    g.setColour (asTemplate ? juce::Colours::black : juce::Colour (0xff2d7eff));
    g.fillEllipse (cx - 3, cy - 3, 6, 6);
    return img;
}

#endif

//==============================================================================
class OrbitAudioApplication  : public juce::JUCEApplication
{
public:
    OrbitAudioApplication() {}

    const juce::String getApplicationName() override       { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override             { return false; }

    void initialise (const juce::String& commandLine) override
    {
        juce::ignoreUnused (commandLine);
#if JUCE_MAC
        macStyleLookAndFeel = std::make_unique<MacOSStyleLookAndFeel>();
#else
        macStyleLookAndFeel = std::make_unique<juce::LookAndFeel_V4> (juce::LookAndFeel_V4::getLightColourScheme());
#endif
        juce::LookAndFeel::setDefaultLookAndFeel (macStyleLookAndFeel.get());
        mainComponent = std::make_unique<MainComponent>();

#if JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX || JUCE_BSD
        trayIcon.reset (new OrbitTrayIcon (*this));
        controlPanel = std::make_unique<ControlPanelWindow> (*mainComponent);
#endif
    }

    void shutdown() override
    {
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
#if JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX || JUCE_BSD
        controlPanel = nullptr;
        trayIcon = nullptr;
#endif
        mainComponent = nullptr;
        macStyleLookAndFeel = nullptr;
    }

    void systemRequestedQuit() override    { quit(); }
    void anotherInstanceStarted (const juce::String&) override {}

    void showControlPanel()
    {
        if (controlPanel != nullptr)
            controlPanel->showAndFocus();
    }

    void hideControlPanel()
    {
        if (controlPanel != nullptr)
            controlPanel->setVisible (false);
    }

    bool isControlPanelVisible() const
    {
        return controlPanel != nullptr && controlPanel->isVisible();
    }

    void toggleControlPanel()
    {
        if (isControlPanelVisible())
            hideControlPanel();
        else
            showControlPanel();
    }

private:
    std::unique_ptr<juce::LookAndFeel> macStyleLookAndFeel;
    std::unique_ptr<MainComponent> mainComponent;

#if JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX || JUCE_BSD
    //==========================================================================
    class OrbitTrayIcon  : public juce::SystemTrayIconComponent,
                           private juce::Timer
    {
    public:
        OrbitTrayIcon (OrbitAudioApplication& app)
            : owner (app)
        {
            setIconImage (createMenuBarIcon (false), createMenuBarIcon (true));
            setIconTooltip ("OrbitAudio - 3D/8D spatializer");
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            wasRightClick = e.mods.isRightButtonDown();
            juce::Process::makeForegroundProcess();
            startTimer (50);
        }

        void timerCallback() override
        {
            stopTimer();
            if (wasRightClick)
            {
                juce::PopupMenu m;
                m.addItem (1, "Open OrbitAudio");
                m.addSeparator();
                m.addItem (2, "Quit");
                m.showMenuAsync (juce::PopupMenu::Options(),
                    juce::ModalCallbackFunction::forComponent (menuCallback, this));
            }
            else
            {
                owner.toggleControlPanel();
            }
        }

    private:
        bool wasRightClick = false;
        OrbitAudioApplication& owner;

        static void menuCallback (int id, OrbitTrayIcon* self)
        {
            if (self == nullptr) return;
            if (id == 1)
                self->owner.showControlPanel();
            else if (id == 2)
                juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbitTrayIcon)
    };

    //==========================================================================
    class ControlPanelWindow  : public juce::DocumentWindow
    {
    public:
        ControlPanelWindow (MainComponent& content)
            : DocumentWindow ("OrbitAudio",
                              content.getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId),
                              DocumentWindow::closeButton),
              contentComp (content)
        {
            setContentNonOwned (&contentComp, true);
            setUsingNativeTitleBar (false);
            setResizable (true, true);
            auto size = contentComp.getPreferredSize();
            setSize (size.x, size.y);
            setResizeLimits (500, 300, 900, 1000);
            setVisible (false);
            setDropShadowEnabled (true);
            contentComp.setOnPreferredSizeChanged ([this]
            {
                auto s = contentComp.getPreferredSize();
                setSize (s.x, s.y);
            });
        }

        void showAndFocus()
        {
            positionAtTopRight();
            setVisible (true);
            toFront (true);
        }

        void closeButtonPressed() override
        {
            setVisible (false);
        }

    private:
        MainComponent& contentComp;

        void positionAtTopRight()
        {
            auto bounds = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
            const int margin = 20;
            setTopLeftPosition (bounds.getRight() - getWidth() - margin,
                               bounds.getY() + margin);
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlPanelWindow)
    };

    std::unique_ptr<juce::SystemTrayIconComponent> trayIcon;
    std::unique_ptr<ControlPanelWindow> controlPanel;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbitAudioApplication)
};

//==============================================================================
START_JUCE_APPLICATION (OrbitAudioApplication)
