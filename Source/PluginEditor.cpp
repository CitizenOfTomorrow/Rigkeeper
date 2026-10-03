/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
RigkeeperAudioProcessorEditor::RigkeeperAudioProcessorEditor (RigkeeperAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p) 
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    DBG("creatingEditor RigkeeperAudioProcessorEditor ");
    auto size = audioProcessor.getSavedSize();
    setResizable(true, true);
    setSize (size.x, size.y);
    
    setResizeLimits(MIN_WIDTH, MIN_HEIGHT, MAX_WIDTH, MAX_HEIGHT);
    addAndMakeVisible(rigkCmpt);
    rigkCmpt.setBounds(getLocalBounds());
    rigkCmpt.setFont(Font(16.0f, Font::bold));
    rigkCmpt.setJustificationType(Justification::centred);
    rigkCmpt.addListener(this); // add listener

    //check if rig was restored from daw or there is dragged rig - display default text otherwise
    if (audioProcessor.rigFileNameValue.toString().isNotEmpty())
    {
        rigkCmpt.setRigFileName(audioProcessor.rigFileNameValue.toString());
        rigkCmpt.setRigContent(audioProcessor.rigFileContentValue.toString());
        rigkCmpt.setText(audioProcessor.rigFileNameValue.toString(), dontSendNotification);
    }
    else   
	{
        rigkCmpt.displayInitialText(); //set default message
    }
    addChildComponent(rigView);
    updateRigView();
  
 
    
       
}

RigkeeperAudioProcessorEditor::~RigkeeperAudioProcessorEditor()
{
}

//==============================================================================
void RigkeeperAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    DBG("painting Editor ");
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    
    
}

void RigkeeperAudioProcessorEditor::resized()
{
    DBG("resizing Editor ");
    auto area = getLocalBounds();
    if (rigView.hasRig())
    {
        rigkCmpt.setBounds(area.removeFromTop(64));
        rigView.setBounds(area.reduced(16, 8));
    }
    else
        rigkCmpt.setBounds(area);
    audioProcessor.setSavedSize({ getWidth(), getHeight() });
}

void RigkeeperAudioProcessorEditor::updateRigView()
{
    MemoryBlock data;
    rigfile::Rig rig;
    const bool parsed = data.fromBase64Encoding(audioProcessor.rigFileContentValue.toString())
        && rigfile::parse(static_cast<const std::uint8_t*>(data.getData()), data.getSize(), rig);
    rigView.setRig(parsed ? &rig : nullptr);
    rigView.setVisible(parsed);
    resized();
}






