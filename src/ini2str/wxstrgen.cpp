/**
 * @file
 *
 * @author OmniBlade
 *
 * @brief wxWidgets front end for string table conversion.
 *
 * @copyright Chronoshift is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include <wx/wx.h>
#include <wx/filesys.h>
#include <wx/fs_mem.h>
#include <wx/xrc/xmlres.h>
#include <wx/xrc/xh_all.h>
#include <wx/filepicker.h>

#include "ini2str.h"
#include "wxstrgenui.h"

class StrGenFrame: public MainFrame
{
public:
    StrGenFrame(wxWindow *parent = NULL) : MainFrame(parent) {}

    void OnCreateIni(wxCommandEvent &event);
    void OnCreateStr(wxCommandEvent &event);
    void OnExitToolOrMenuCommand(wxCommandEvent &event);

    wxDECLARE_EVENT_TABLE();
};

// clang-format off
wxBEGIN_EVENT_TABLE(StrGenFrame, wxFrame)
EVT_MENU(wxID_EXIT,  StrGenFrame::OnExitToolOrMenuCommand)
EVT_BUTTON(XRCID("m_buttonToIni"), StrGenFrame::OnCreateIni)
EVT_BUTTON(XRCID("m_buttonToStr"), StrGenFrame::OnCreateStr)
wxEND_EVENT_TABLE()
// clang-format on



void StrGenFrame::OnCreateIni(wxCommandEvent &event)
{
    if (!m_filePickerString->GetFileName().IsOk() || !m_filePickerString->GetFileName().FileExists()) {
        m_statusBar->PopStatusText();
        m_statusBar->PushStatusText("Choose a file to convert to into an ini file");
        return;
    }

    if (!m_filePickerINI->GetFileName().IsOk()) {
        m_statusBar->PopStatusText();
        m_statusBar->PushStatusText("Choose a file name for the new ini file");
        return;
    }

    m_statusBar->PopStatusText();
    m_statusBar->PushStatusText("Converting file...");
    StringTable_To_INI(m_filePickerString->GetFileName().GetFullPath(), m_filePickerINI->GetFileName().GetFullPath(), m_textCtrl1->GetLineText(0));
    m_statusBar->PopStatusText();
    m_statusBar->PushStatusText("File converted");
}

void StrGenFrame::OnCreateStr(wxCommandEvent &event)
{
    if (!m_filePickerINI->GetFileName().IsOk() || !m_filePickerINI->GetFileName().FileExists()) {
        m_statusBar->PopStatusText();
        m_statusBar->PushStatusText("Choose a file to convert to into a string file");
        return;
    }

    if (!m_filePickerString->GetFileName().IsOk()) {
        m_statusBar->PopStatusText();
        m_statusBar->PushStatusText("Choose a file name for the new string file");
        return;
    }

    m_statusBar->PopStatusText();
    m_statusBar->PushStatusText("Converting file...");
    INI_To_StringTable(m_filePickerINI->GetFileName().GetFullPath(),
        m_filePickerString->GetFileName().GetFullPath(),
        m_textCtrl1->GetLineText(0));
    m_statusBar->PopStatusText();
    m_statusBar->PushStatusText("File converted");
}

void StrGenFrame::OnExitToolOrMenuCommand(wxCommandEvent &event)
{
    Close(true);
}

// This is our application implementation.
class StrGenApp : public wxApp
{
public:
    // Override base class virtuals:
    // wxApp::OnInit() is called on application startup and is a good place
    // for the app initialization (doing it here and not in the ctor
    // allows to have an error return: if OnInit() returns false, the
    // application terminates)
    virtual bool OnInit();
};

IMPLEMENT_APP(StrGenApp)

// This is effectively our "main" function since the real one is behind IMPLEMENT_APP
bool StrGenApp::OnInit()
{
    if (!wxApp::OnInit()) {
        return false;
    }

    wxXmlResource *res = wxXmlResource::Get();
    res->AddHandler(new wxUnknownWidgetXmlHandler);
    res->AddHandler(new wxBitmapXmlHandler);
    res->AddHandler(new wxIconXmlHandler);
    res->AddHandler(new wxDialogXmlHandler);
    res->AddHandler(new wxPanelXmlHandler);
    res->AddHandler(new wxSizerXmlHandler);
    res->AddHandler(new wxFrameXmlHandler);
    res->AddHandler(new wxScrolledWindowXmlHandler);
    res->AddHandler(new wxStatusBarXmlHandler);
    res->AddHandler(new wxStaticTextXmlHandler);
    res->AddHandler(new wxFilePickerCtrlXmlHandler);
    res->AddHandler(new wxTextCtrlXmlHandler);
    res->AddHandler(new wxButtonXmlHandler);
    InitXmlResource();
    wxFrame *frame = new StrGenFrame;
    frame->Show();

    return true;
}
