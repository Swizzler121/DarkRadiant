#include "EntityPropertyEditor.h"

#include "i18n.h"
#include "iscenegraph.h"
#include "iundo.h"
#include "ientity.h"

#include "PropertyEditorFactory.h"

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/artprov.h>
#include <wx/sizer.h>

#include "ui/common/EntityChooser.h"

namespace ui
{

EntityPropertyEditor::EntityPropertyEditor(wxWindow* parent, IEntitySelection& entities, const std::string& name) :
	PropertyEditor(entities),
	_key(name)
{
	constructBrowseButtonPanel(parent, _("Choose target entity..."),
		PropertyEditorFactory::getBitmapFor("entity"));
}

void EntityPropertyEditor::onBrowseButtonClick()
{
	// Use a new dialog window to get a selection from the user
    auto previousValue = _entities.getSharedKeyValue(_key, false);
	std::string selection = EntityChooser::ChooseEntity(previousValue);

	// Only apply non-empty selections if the value has actually changed
	if (!selection.empty() && selection != previousValue)
	{
		UndoableCommand cmd("changeKeyValue");

		// Apply the change
        setKeyValue(_key, selection);
	}
}

} // namespace ui
