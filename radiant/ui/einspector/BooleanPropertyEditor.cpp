#include "BooleanPropertyEditor.h"

#include "ientity.h"

#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/sizer.h>

namespace ui
{

// Constructor. Create the widgets here
BooleanPropertyEditor::BooleanPropertyEditor(wxWindow* parent, IEntitySelection& entities,
											 const std::string& name)
: PropertyEditor(entities),
  _checkBox(nullptr),
  _key(name)
{
	// Construct the main widget (will be managed by the base class)
	wxPanel* mainVBox = new wxPanel(parent, wxID_ANY);
	mainVBox->SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// Register the main widget in the base class
	setMainWidget(mainVBox);

	// Create the checkbox with correct initial state, and connect up the
	// toggle callback
	_checkBox = new wxCheckBox(mainVBox, wxID_ANY, name);

	updateFromEntity();

	_checkBox->Bind(wxEVT_CHECKBOX, &BooleanPropertyEditor::_onToggle, this);

	mainVBox->GetSizer()->Add(_checkBox, 0, wxALIGN_CENTER_VERTICAL);
}

void BooleanPropertyEditor::updateFromEntity()
{
	_checkBox->SetValue(_entities.getSharedKeyValue(_key, false) == "1");
}

void BooleanPropertyEditor::_onToggle(wxCommandEvent& ev)
{
	// Set the key based on the checkbutton state
	setKeyValue(_key, _checkBox->IsChecked() ? "1" : "0");
}

} // namespace
