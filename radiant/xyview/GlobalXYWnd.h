#pragma once

#include <list>
#include <map>
#include "iorthoview.h"
#include "iclipper.h"
#include "iregistry.h"
#include "icommandsystem.h"
#include "imousetoolmanager.h"

#include "XYWnd.h"

class wxMouseEvent;

namespace ui
{

class XYWndManager : 
    public IXWndManager
{
	// Store an indexed map of XYWnds. When one is deleted, it will notify
	// the XYWndManager of its index so that it can be removed from the map
	typedef std::map<int, XYWndPtr> XYWndMap;
	XYWndMap _xyWnds;

	// The active XYWnd
	XYWndPtr _activeXY;

	// True, if the view is moved when the mouse cursor exceeds the view window borders
	bool _chaseMouse;
    int _chaseMouseCap;

	bool _camXYUpdate;

	// The various display settings for xyviews
	bool _showCrossHairs;
	bool _showGrid;
	bool _showSizeInfo;
	bool _showBlocks;
	bool _showCoordinates;
	bool _showOutline;
	bool _showAxes;
	bool _showWorkzone;
	bool _zoomCenteredOnMouseCursor;

	unsigned int _defaultBlockSize;
    int _fontSize;
    IGLFont::Style _fontStyle;

    std::size_t _maxZoomFactor;

private:

	// Get a unique ID for the XYWnd map
	int getUniqueID() const;
    void observeKey(const std::string&);
	void refreshFromRegistry();

public:

	// Constructor
	XYWndManager();

	// Returns the state of the xy view preferences
	bool chaseMouse() const;
    int chaseMouseCap() const;
	bool camXYUpdate() const;
	bool showCrossHairs() const;
	bool showGrid() const;
	bool showBlocks() const;
	bool showCoordinates() const;
	bool showOutline() const;
	bool showAxes() const;
	bool showWorkzone() const;
	bool showSizeInfo() const;
	int fontSize() const;
    IGLFont::Style fontStyle() const;
    float maxZoomFactor() const;
    bool zoomCenteredOnMouseCursor() const;

	unsigned int defaultBlockSize() const;

	// Passes a draw call to each allocated view, set force to true 
    // to redraw immediately instead of queueing the draw.
	void updateAllViews(bool force = false) override;

	// Free all the allocated views from the heap
	void destroyViews() override;

	// Saves the current state of all open views to the registry
	void saveState();
	// Restores the xy windows according to the state saved in the XMLRegistry
	void restoreState();

	XYWndPtr getActiveXY() const;

	/**
	 * Set the given XYWnd to active state.
	 *
	 * @param id
	 * Unique ID of the XYWnd to set as active.
	 */
	void setActiveXY(int id);

	// Shortcut commands for connect view the EventManager
	void setActiveViewXY(const cmd::ArgumentList& args); // top view
	void setActiveViewXZ(const cmd::ArgumentList& args); // side view
	void setActiveViewYZ(const cmd::ArgumentList& args); // front view
	void splitViewFocus(const cmd::ArgumentList& args); // Re-position all available views
	void zoom100(const cmd::ArgumentList& args); // Sets the scale of all windows to 1
	void focusActiveView(const cmd::ArgumentList& args); // sets the focus of the active view

	// Sets the origin of all available views
	void setOrigin(const Vector3& origin) override;
	Vector3 getActiveViewOrigin() override;

	// Sets the scale of all available views
	void setScale(float scale) override;

	// Zooms the currently active view in/out
	void zoomIn(const cmd::ArgumentList& args);
	void zoomOut(const cmd::ArgumentList& args);

	// Positions the view of all available views / the active view
	void positionAllViews(const Vector3& origin) override;
	void positionActiveView(const Vector3& origin) override;

	// Returns the view type of the currently active view
	EViewType getActiveViewType() const override;
	void setActiveViewType(EViewType viewType) override;

	IOrthoView& getActiveView() override;
	IOrthoView& getViewByType(EViewType viewType) override;

	void toggleActiveView(const cmd::ArgumentList& args);

	// Retrieves the pointer to the first view matching the given view type
	// @returns: NULL if no matching window could be found, the according pointer otherwise
	XYWndPtr getView(EViewType viewType);

	/**
	 * Create a non-floating (embedded) ortho view. DEPRECATED
	 */
	XYWndPtr createEmbeddedOrthoView();

	/**
	 * Create a non-floating (embedded) orthoview of the given type
	 */
	XYWndPtr createEmbeddedOrthoView(EViewType viewType, wxWindow* parent);

	/**
	 * Create a new floating ortho view, as a child of the main window.
	 */
	XYWndPtr createFloatingOrthoView(EViewType viewType);

	/**
	 * Parameter-less wrapper for createFloatingOrthoView(), for use by the
	 * event manager. The default orientation of XY is used.
	 */
	void createXYFloatingOrthoView(const cmd::ArgumentList& args);

	/**
	 * greebo: This removes a certain orthoview ID, usually initiating
	 * destruction of the XYWnd/FloatingOrthoView object.
	 */
	void destroyXYWnd(int id);

    MouseToolStack getMouseToolsForEvent(wxMouseEvent& ev);
    void foreachMouseTool(const std::function<void(const MouseToolPtr&)>& func);

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	/* greebo: This function determines the point currently being "looked" at, it is used for toggling the ortho views
	 * If something is selected the center of the selection is taken as new origin, otherwise the camera
	 * position is considered to be the new origin of the toggled orthoview. */
	Vector3 getFocusPosition();

	// Construct the orthoview preference page and add it to the given group
	void constructPreferences();

	// Registers all the XY commands in the EventManager
	void registerCommands();

}; // class XYWndManager

} // namespace

// Use this method to access the global XYWnd manager class
ui::XYWndManager& GlobalXYWnd();
