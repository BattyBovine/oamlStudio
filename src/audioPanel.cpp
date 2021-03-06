//-----------------------------------------------------------------------------
// Copyright (c) 2015-2016 Marcelo Fernandez
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oamlCommon.h"

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/listctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/scrolbar.h>
#include <wx/sizer.h>
#include <wx/frame.h>
#include <wx/textctrl.h>
#include <wx/filename.h>
#include <wx/filehistory.h>
#include <wx/config.h>


AudioPanel::AudioPanel(wxFrame* parent, int index, std::string name, wxString labelStr, bool mode) : wxPanel(parent) {
	panelIndex = index;
	trackName = name;
	sfxMode = mode;

	vSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText *staticText = new wxStaticText(this, wxID_ANY, labelStr, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
	staticText->SetBackgroundColour(wxColour(0xD0, 0xD0, 0xD0));
	vSizer->Add(staticText, 0, wxALL | wxEXPAND | wxGROW, 5);

	sizer = new wxBoxSizer(wxHORIZONTAL);
	vSizer->Add(sizer);

	SetSizer(vSizer);

	Bind(wxEVT_PAINT, &AudioPanel::OnPaint, this);
	Bind(wxEVT_RIGHT_UP, &AudioPanel::OnRightUp, this);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &AudioPanel::OnMenuEvent, this);

//	SetMinSize(wxSize(240, -1));
}

void AudioPanel::OnPaint(wxPaintEvent& WXUNUSED(evt)) {
	wxPaintDC dc(this);

	wxSize size = GetSize();
	int x2 = size.GetWidth();
	int y2 = size.GetHeight();

	dc.SetPen(wxPen(wxColour(0, 0, 0), 4));
	dc.DrawLine(0,  0,  0,  y2);
	dc.DrawLine(x2, 0,  x2, y2);
	dc.DrawLine(0,  0,  x2, 0);
	dc.DrawLine(0,  y2, x2, y2);
}

void AudioPanel::AddAudio(std::string audioName) {
	AudioFilePanel *afp = new AudioFilePanel(trackName, audioName, sfxMode, (wxFrame*)this);
	std::vector<std::string> list;

	filePanels.push_back(afp);
	studioApi->AudioGetAudioFileList(trackName, audioName, list);
	for (std::vector<std::string>::iterator it=list.begin(); it<list.end(); ++it) {
		afp->AddWaveform(*it);
	}

	sizer->Add(afp, 0, wxALL, 5);
	Layout();

	wxCommandEvent event(EVENT_UPDATE_LAYOUT);
	wxPostEvent(GetParent(), event);
}

void AudioPanel::RemoveAudio(std::string filename) {
	for (std::vector<AudioFilePanel*>::iterator it=filePanels.begin(); it<filePanels.end(); ++it) {
		AudioFilePanel *afp = *it;
		afp->RemoveWaveform(filename);
		if (afp->IsEmpty()) {
			sizer->Detach((wxWindow*)afp);
			filePanels.erase(it);
			delete afp;

			studioApi->AudioRemove(trackName, filename);
			break;
		}
	}

	Layout();

	// Mark the project dirty
	wxCommandEvent event(EVENT_SET_PROJECT_DIRTY);
	wxPostEvent(GetParent(), event);
}

void AudioPanel::AddAudioPath(wxString path) {
	wxFileName filename(path);

	filename.MakeRelativeTo(wxString(projectPath));
	std::string fname = filename.GetFullPath().ToStdString();

	int type = 2;
	switch (panelIndex) {
		case 0: type = 1; break;
		case 1: type = 2; break;
		case 2: type = 4; break;
	}

	std::string name;
	for (int i=0; i<1000; i++) {
		char str[1024];
		snprintf(str, 1024, "audio%d", i);
		name = str;
		if (studioApi->AudioExists(trackName, name) == false) {
			break;
		}
	}

	studioApi->AudioNew(trackName, name, type);
	studioApi->AudioAddAudioFile(trackName, name, fname);

	AddAudio(name);

	// Mark the project dirty
	wxCommandEvent event2(EVENT_SET_PROJECT_DIRTY);
	wxPostEvent(GetParent(), event2);
}

void AudioPanel::AddAudioDialog() {
	wxFileDialog openFileDialog(this, _("Open audio file"), wxEmptyString, "", "Audio files (*.wav;*.aif;*.ogg)|*.aif;*.aiff;*.wav;*.wave;*.ogg", wxFD_OPEN|wxFD_FILE_MUST_EXIST|wxFD_MULTIPLE);
	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return;

	wxArrayString paths;
	openFileDialog.GetPaths(paths);
	for (size_t i=0; i<paths.GetCount(); i++) {
		AddAudioPath(paths.Item(i));
	}
}

void AudioPanel::OnMenuEvent(wxCommandEvent& event) {
	switch (event.GetId()) {
		case ID_AddAudio:
			AddAudioDialog();
			break;
	}
}

void AudioPanel::OnRightUp(wxMouseEvent& WXUNUSED(event)) {
	wxMenu menu(wxT(""));
	menu.Append(ID_AddAudio, wxT("&Add Audio"));
	PopupMenu(&menu);
}

void AudioPanel::UpdateTrackName(std::string newName) {
	trackName = newName;
}

void AudioPanel::UpdateAudioName(std::string oldName, std::string newName) {
	for (std::vector<AudioFilePanel*>::iterator it=filePanels.begin(); it<filePanels.end(); ++it) {
		AudioFilePanel *afp = *it;
		afp->UpdateAudioName(oldName, newName);
	}
}

