#ifndef ENTITYINSPECTOR_H_
#define ENTITYINSPECTOR_H_

#include "gtkutil/idledraw.h"
#include "xmlutil/Node.h"
#include "xmlutil/Document.h"

#include "iselection.h"
#include "ientity.h"
#include "igamedescriptor.h"

#include "PropertyEditor.h"

#include <gtk/gtk.h>
#include <iostream>
#include <set>

namespace ui {

// CONSTANTS AND TYPES

namespace {

    const int TREEVIEW_MIN_WIDTH = 220;
    const int TREEVIEW_MIN_HEIGHT = 60;
    const int PROPERTYEDITORPANE_MIN_HEIGHT = 120;
    
    const std::string NO_VALUE_STRING = "";
    const char* ADVANCED_BUTTON_STRING = "All properties...";

    const std::string UNRECOGNISED_PROPERTIES_PREFIX = "<span foreground=\"red\">";
    const std::string UNRECOGNISED_PROPERTIES_SUFFIX = " unrecognised properties</span>";

    typedef std::set<std::string> KnownPropertySet;

}

/* The EntityInspector class represents the GTK dialog for editing properties
 * on the selected game entity. The class is implemented as a singleton and
 * contains a method to return the current instance.
 */

class EntityInspector
{
private:

    // TreeView column numbers
    
    enum {
        PROPERTY_NAME_COLUMN,
        PROPERTY_VALUE_COLUMN,
        PROPERTY_TYPE_COLUMN,
        PROPERTY_OPTIONS_COLUMN,
        TEXT_COLOUR_COLUMN,
        PROPERTY_ICON_COLUMN,
        N_COLUMNS
    };

	// Currently selected entity
	Entity* _selectedEntity;

    // The Gtk dialog widgets

    GtkWidget* _widget; 
    
    GtkWidget* _editorFrame;
    GtkWidget* _selectionTreeView;
    
    GtkTreeStore* _treeStore;
    GtkWidget* _treeView;

    GtkWidget* _unrecognisedPropertiesMessage;

	// Currently displayed PropertyEditor
	PropertyEditor* _currentPropertyEditor;

    // GtkUtil IdleDraw class. This allows redraw calls to be scheduled for
    // when GTK is idle.
    IdleDraw _idleDraw;

	/* Property storage. The base Property is a simple
	 * data structure containing a given property's name, its type and its
	 * option string. These Property structures are then assembled
	 * into a vector which is stored in the PropertyCategoryMap keyed on the
	 * category name ("Light", "Model") etc.
	 * 
	 * A Property therefore represents a single row in the tree view
	 * widget, while the PropertyCategoryMap maps expandable category names
	 * onto the vector of rows which should appear in that category.
	 */

	struct Property {
		std::string name; 		// e.g. "light_radius"
		std::string type; 		// e.g. "vector3"
		std::string options;	// property-specific option string
	};

	typedef std::vector<Property*> PropertyCategory;

	typedef std::map<const std::string, PropertyCategory*> PropertyCategoryMap;

	// The static category map
	static PropertyCategoryMap _categoryMap;
    
    // The set of known Property types.
    KnownPropertySet _knownProperties;

private:

    // Utility functions to construct the Gtk components

    void constructUI();

    GtkWidget* createDialogPane(); // bottom widget pane 
    GtkWidget* createTreeViewPane(); // tree view for selecting attributes

    // GTK CALLBACKS
    // Must be static as they are called from a C-based API
    static void callbackTreeSelectionChanged(GtkWidget* widget, EntityInspector* self);
    static gboolean treeWalkFunc(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer);
    static void callbackAdvancedButtonClicked(GtkWidget* widget, EntityInspector* self);

    // Routines to populate the TreeStore with the keyvals attached to the
    // currently-selected object. 
    void populateTreeModel(); // add to empty TreeModel
    void refreshTreeModel(); // refresh values in existing TreeModel

    // Update the PropertyEditor pane with the correct PropertyEditor.
    void updatePropertyEditor();
    
	// Update the currently selected entity pointer. This function returns true
	// if a single Entity is selected, and false if either a non-Entity or more
	// than one object is selected.
	bool updateSelectedEntity();

	// Utility function to create a PropertyCategory object and add it to the
	// map.
	static void makePropertyCategory(xml::Node& node);

public:

    // Constructor
    EntityInspector():
        // Set the IdleDraw instance to call the doRedraw function when
        // required
        _idleDraw(MemberCaller<EntityInspector, &EntityInspector::callbackRedraw>(*this)) {}

    // Return or create the singleton instance
    static EntityInspector* getInstance();

    // Get the Gtk Widget for display in the main application
    GtkWidget* getWidget();

	// Use libxml2 to parse the <entityInspector> subtree of the .game file. 
	// Invoked from CGameDescription constructor in preferences.cpp
	static void parseXml(xml::Document doc);

    // Inform the IdleDraw to invoke a redraw when idle
    void queueDraw();
    
    // Redraw the GUI elements. Called by the IdleDraw object when GTK is idle
    // and a queueDraw request has been passed.
    void callbackRedraw();
    
    // Static class function to instigate a redraw. This is passed as a pointer
    // to the GlobalEntityCreator's setKeyValueChangedFunc function.
    static void redrawInstance();

    // Function to call when the current Selection is changed by the selection
    // system. Internally this function will just stimulate a redraw, but it
    // must take a reference to the Selectable object.
    static void selectionChanged(const Selectable&);

};

} // namespace ui

#endif /*ENTITYINSPECTOR_H_*/
