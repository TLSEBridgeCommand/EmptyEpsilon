#include "playerInfo.h"
#include "spaceObjects/playerSpaceship.h"
#include "commsOverlay.h"
#include "gui/gui2_panel.h"
#include "gui/gui2_progressbar.h"
#include "gui/gui2_button.h"
#include "gui/gui2_label.h"
#include "gui/gui2_scrolltext.h"
#include "gui/gui2_listbox.h"
#include "gui/gui2_textentry.h"
// NB start of Harry's patch for comms
#include "gui/gui2_canvas.h"

#include "onScreenKeyboard.h"

GuiCommsOverlay::GuiCommsOverlay(GuiContainer* owner)
//NB adding new variable
: GuiElement(owner, "COMMS_OVERLAY"), chat_open_last_update(false)
{
    // Panel for reporting outgoing hails.
    opening_box = new GuiPanel(owner, "COMMS_OPENING_BOX");
    opening_box->hide()->setSize(800, 100)->setPosition(0, -250, ABottomCenter);
    (new GuiLabel(opening_box, "COMMS_OPENING_LABEL", tr("comms","Opening communications..."), 40))->setSize(GuiElement::GuiSizeMax, 50)->setPosition(0, 0, ATopCenter);
    opening_progress = new GuiProgressbar(opening_box, "COMMS_OPENING_PROGRESS", PlayerSpaceship::comms_channel_open_time, 0.0, 0.0);
    opening_progress->setSize(500, 40)->setPosition(50, -10, ABottomLeft);

    // Cancel button closes the communication.
    opening_cancel = new GuiButton(opening_box, "COMMS_OPENING_CANCEL", tr("comms","Cancel"), []()
    {
        if (my_spaceship)
            my_spaceship->commandCloseTextComm();
    });
    opening_cancel->setSize(200, 40)->setPosition(-50, -10, ABottomRight);

    // Panel for reporting incoming hails.
    hailed_box = new GuiPanel(owner, "COMMS_BEING_HAILED_BOX");
    hailed_box->hide()->setSize(800, 140)->setPosition(0, -250, ABottomCenter);
    hailed_label = new GuiLabel(hailed_box, "COMMS_BEING_HAILED_LABEL", "..", 40);
    hailed_label->setSize(GuiElement::GuiSizeMax, 50)->setPosition(0, 20, ATopCenter);

    // Buttons to answer or ignore hails.
    hailed_answer = new GuiButton(hailed_box, "COMMS_BEING_HAILED_ANSWER", tr("comms","Answer"), []() {
        if (my_spaceship)
            my_spaceship->commandAnswerCommHail(true);
    });
    hailed_answer->setSize(300, 50)->setPosition(20, -20, ABottomLeft);

    hailed_ignore = new GuiButton(hailed_box, "COMMS_BEING_HAILED_IGNORE", tr("comms","Ignore"), []() {
        if (my_spaceship)
            my_spaceship->commandAnswerCommHail(false);
    });
    hailed_ignore->setSize(300, 50)->setPosition(-20, -20, ABottomRight);

    // Panel for unresponsive hails.
    no_response_box = new GuiPanel(owner, "COMMS_OPENING_BOX");
    no_response_box->hide()->setSize(800, 70)->setPosition(0, -250, ABottomCenter);
    (new GuiLabel(no_response_box, "COMMS_NO_REPONSE_LABEL", tr("comms","No reply..."), 40))->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax)->setPosition(0, 0, ATopLeft);

    // Button to acknowledge unresponsive hails.
    (new GuiButton(no_response_box, "COMMS_NO_REPLY_OK", tr("comms","Ok"), []() {
        if (my_spaceship)
            my_spaceship->commandCloseTextComm();
    }))->setSize(100, 50)->setPosition(-20, -10, ABottomRight);

    // Panel for broken communications.
    broken_box = new GuiPanel(owner, "COMMS_BROKEN_BOX");
    broken_box->hide()->setSize(800, 70)->setPosition(0, -250, ABottomCenter);
    (new GuiLabel(broken_box, "COMMS_BROKEN_LABEL", tr("comms","Communications were suddenly cut"), 40))->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax)->setPosition(0, 0, ATopLeft);

    // Button to acknowledge broken communications.
    (new GuiButton(broken_box, "COMMS_BROKEN_OK", tr("comms","Ok"), []() {
        if (my_spaceship)
            my_spaceship->commandCloseTextComm();
    }))->setSize(100, 50)->setPosition(-20, -10, ABottomRight);

    // Panel for communications closed by the other object.
    closed_box = new GuiPanel(owner, "COMMS_CLOSED_BOX");
    closed_box->hide()->setSize(800, 70)->setPosition(0, -250, ABottomCenter);
    (new GuiLabel(closed_box, "COMMS_BROKEN_LABEL", tr("comms","Communications channel closed"), 40))->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax)->setPosition(0, 0, ATopLeft);

    // Button to acknowledge closed communications.
    (new GuiButton(closed_box, "COMMS_CLOSED_OK", tr("comms","Ok"), []() {
        if (my_spaceship)
            my_spaceship->commandCloseTextComm();
    }))->setSize(100, 50)->setPosition(-20, -10, ABottomRight);

    // Panel for chat communications with GMs and other player ships.
    chat_comms_box = new GuiPanel(owner, "COMMS_CHAT_BOX");
    chat_comms_box->hide()->setSize(800, 600)->setPosition(0, -100, ABottomCenter);

    // Title bar with minimize and close buttons
    GuiElement* title_bar = new GuiElement(chat_comms_box, "CHAT_TITLE_BAR");
    title_bar->setSize(GuiElement::GuiSizeMax, 40)->setPosition(0, 0, ATopLeft);

    // Title label showing which ship you're communicating with
    chat_comms_title_label = new GuiLabel(title_bar, "COMMS_CHAT_TITLE_LABEL", "", 30);
    chat_comms_title_label->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax)->setPosition(0, 0, ATopLeft);
    chat_comms_title_label->setAlignment(ACenter);

    // Minimize button
    chat_comms_minimize_button = new GuiToggleButton(title_bar, "CHAT_MINIMIZE", "_", [this](bool value) {
        minimizeChat(value);
    });
    chat_comms_minimize_button->setSize(50, GuiElement::GuiSizeMax)->setPosition(-60, 0, ATopRight);

    // Close button
    chat_comms_close_button = new GuiButton(title_bar, "CHAT_CLOSE", "x", [this]() {
        if (my_spaceship)
            my_spaceship->commandCloseTextComm();
    });
    chat_comms_close_button->setSize(50, GuiElement::GuiSizeMax)->setPosition(-10, 0, ATopRight);

    // Content area for chat
    chat_comms_content = new GuiElement(chat_comms_box, "CHAT_CONTENT");
    chat_comms_content->setPosition(0, 40, ATopLeft)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);

    // Message entry field for chat.
    chat_comms_message_entry = new GuiTextEntry(chat_comms_content, "COMMS_CHAT_MESSAGE_ENTRY", "");
    chat_comms_message_entry->setPosition(20, -20, ABottomLeft)->setSize(640, 50);
    chat_comms_message_entry->enterCallback([this](string text){
        if (my_spaceship)
        {
            my_spaceship->commandSendCommPlayer(chat_comms_message_entry->getText());
        }
        chat_comms_message_entry->setText("");
    });


    // Text of incoming chat messages.
    chat_comms_text = new GuiScrollText(chat_comms_content, "COMMS_CHAT_TEXT", "");
    chat_comms_text->enableAutoScrollDown()->setPosition(20, 0, ATopLeft)->setSize(760, 500);

    // Button to send a message.
    chat_comms_send_button = new GuiButton(chat_comms_content, "SEND_BUTTON", tr("comms","Send"), [this]() {
        if (my_spaceship)
            my_spaceship->commandSendCommPlayer(chat_comms_message_entry->getText());
        chat_comms_message_entry->setText("");
    });
    chat_comms_send_button->setPosition(-20, -20, ABottomRight)->setSize(120, 50);

    if (!engine->getObject("mouseRenderer")) //If we are a touch screen, add a on screen keyboard.
    {
        OnScreenKeyboardControl* keyboard = new OnScreenKeyboardControl(chat_comms_box, chat_comms_message_entry);
        keyboard->setPosition(20, -20, ABottomLeft)->setSize(760, 200);
        chat_comms_message_entry->setPosition(20, -220, ABottomLeft);
        chat_comms_send_button->setPosition(-20, -220, ABottomRight);
        chat_comms_text->setSize(chat_comms_text->getSize().x, chat_comms_text->getSize().y - 200);
    }

    // Panel for scripted comms with objects.
    script_comms_box = new GuiPanel(owner, "COMMS_SCRIPT_BOX");
    script_comms_box->hide()->setSize(800, 600)->setPosition(0, -100, ABottomCenter);

    // Title label showing which ship you're communicating with
    script_comms_title_label = new GuiLabel(script_comms_box, "COMMS_SCRIPT_TITLE_LABEL", "", 30);
    script_comms_title_label->setSize(GuiElement::GuiSizeMax, 40)->setPosition(0, 10, ATopCenter);
    script_comms_title_label->setAlignment(ACenter);

    script_comms_text = new GuiScrollText(script_comms_box, "COMMS_SCRIPT_TEXT", "");
    script_comms_text->setPosition(20, 30, ATopLeft)->setSize(760, 500);

    // List possible responses to a scripted communication.
    script_comms_options = new GuiListbox(script_comms_box, "COMMS_SCRIPT_LIST", [this](int index, string value) {
        script_comms_options->setOptions({});
        my_spaceship->commandSendComm(index);
    });
    script_comms_options->setPosition(20, -70, ABottomLeft)->setSize(700, 400);

    // Button to close scripted comms.
    script_comms_close = new GuiButton(script_comms_box, "CLOSE_BUTTON", tr("comms","Close"), [this]() {
        script_comms_options->setOptions({});
        if (my_spaceship)
            my_spaceship->commandCloseTextComm();
    });
    script_comms_close->setTextSize(20)->setPosition(-20, -20, ABottomRight)->setSize(150, 50);
    
    // Initialize minimize state
    chat_minimized = false;
    chat_original_height = 600.0f;
    
}

void GuiCommsOverlay::onDraw(sf::RenderTarget& window)
{
    // If we're on a ship, show comms activity on draw.
    if (my_spaceship)
    {
        opening_box->setVisible(my_spaceship->isCommsOpening());
        opening_progress->setValue(my_spaceship->getCommsOpeningDelay());

        hailed_box->setVisible(my_spaceship->isCommsBeingHailed());
        hailed_label->setText(tr("comms","Hailed by ") + my_spaceship->getCommsTargetName());

        no_response_box->setVisible(my_spaceship->isCommsFailed());

        broken_box->setVisible(my_spaceship->isCommsBroken());
        closed_box->setVisible(my_spaceship->isCommsClosed());
// NB pulling in Harry's auto-focus
       if (my_spaceship->isCommsChatOpen() && !chat_open_last_update)
        {
           // Chat window has just opened, let's auto-focus the text input
            auto canvas = dynamic_cast<GuiCanvas*>(getTopLevelContainer());
            if (canvas)
            {
                canvas->focus(chat_comms_message_entry);
            }
        }
        chat_open_last_update = my_spaceship->isCommsChatOpen();

        chat_comms_box->setVisible(my_spaceship->isCommsChatOpen());
        chat_comms_text->setText(my_spaceship->getCommsIncommingMessage());
    
        
        // Set the title label for chat comms - only show when comms are fully established
        if (my_spaceship->isCommsChatOpen())
        {
            // Only show title if we have an actual message (not just the initial "established link" message)
            string message = my_spaceship->getCommsIncommingMessage();
            if (message.find("Opened comms with") == string::npos && message.find("Opened comms to") == string::npos)
            {
                chat_comms_title_label->setText(my_spaceship->getCallSign() + " - Communicating with " + my_spaceship->getCommsTargetName());
            }
            else
            {
                chat_comms_title_label->setText("");
            }
        }

        script_comms_box->setVisible(my_spaceship->isCommsScriptOpen());
        script_comms_text->setText(my_spaceship->getCommsIncommingMessage());
        
        // No title on script comms (buttons/reply-options view) - title only on chat comms where you can enter text
        script_comms_title_label->setText("");

        // Show the scripted comms options. If they've changed, update the lsit
        bool changed = script_comms_options->entryCount() != int(my_spaceship->getCommsReplyOptions().size());
        if (!changed && my_spaceship->getCommsReplyOptions().size() > 0)
            changed = my_spaceship->getCommsReplyOptions()[0] != script_comms_options->getEntryName(0);
        if (changed)
        {
            script_comms_options->setOptions({});
            for(string message : my_spaceship->getCommsReplyOptions())
                script_comms_options->addEntry(message, message);
            int display_options_count = std::min(5, script_comms_options->entryCount());
            script_comms_options->setSize(760, display_options_count * 50);
            script_comms_text->setSize(760, 500 - display_options_count * 50);
        }
    }
}

void GuiCommsOverlay::clearElements()
{
    // Force all panels to hide, in case hiding the overlay doesn't hide its
    // contents on draw.
    opening_box->hide();
    hailed_box->hide();
    no_response_box->hide();
    broken_box->hide();
    closed_box->hide();
    chat_comms_box->hide();
    script_comms_box->hide();
    
    // Clear title labels
    chat_comms_title_label->setText("");
    script_comms_title_label->setText("");
}

void GuiCommsOverlay::minimizeChat(bool minimize)
{
    chat_minimized = minimize;
    chat_comms_minimize_button->setValue(minimize);
    
    // Update button text: "+" when minimized (to maximize), "-" when maximized (to minimize)
    chat_comms_minimize_button->setText(minimize ? "+" : "-");
    
    if (minimize)
    {
        chat_comms_content->hide();
        chat_comms_box->setSize(chat_comms_box->getSize().x, 40); // Just title bar height
    }
    else
    {
        chat_comms_content->show();
        chat_comms_box->setSize(chat_comms_box->getSize().x, chat_original_height);
    }
}

bool GuiCommsOverlay::isChatMinimized() const
{
    return chat_minimized;
}
