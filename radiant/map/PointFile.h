#ifndef POINTFILE_H_
#define POINTFILE_H_

#include <vector>
#include "irender.h"
#include "renderable.h"
#include "math/Vector3.h"

namespace map {

void Pointfile_Construct();

class PointFile : 
	public Renderable, 
	public OpenGLRenderable
{
	// Vector of point coordinates
	typedef std::vector<Vector3> VectorList;
	VectorList _points;
	
	// GL display list pointer for rendering the point path
	int _displayList;
	
	static ShaderPtr _renderstate;

public:

	// Constructor
	PointFile();

	/** greebo: Accessor method containing the singleton instance.
	 */
	static PointFile& Instance();

	// Query whether the point path is currently visible
	bool isVisible() const;

  void generateDisplayList();

  typedef VectorList::const_iterator const_iterator;

  const_iterator begin() const;
  const_iterator end() const;

	/*
	 * Toggle the status of the pointfile rendering. If the pointfile must be
	 * shown, the file is parsed automatically.
	 */
	void show(bool show);

	/*
	 * OpenGL render function (back-end).
	 */
	void render(RenderStateFlags state) const;

	/*
	 * Solid renderable submission function (front-end)
	 */
	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;

	/*
	 * Wireframe renderable submission function (front-end).
	 */
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;

	/** greebo: This sets the camera position to the next/prev leak spot.
	 * 	
	 * @forward: pass true to set to the next leak spot, false for the previous
	 */
	void advance(bool forward);

	/** greebo: Clears the point file vector and hides it, if applicable.
	 */
	void clear();

	static void constructStatic();
	static void destroyStatic();
	
	// Static command targets, these re-route the call to the static instance
	static void nextLeakSpot();
	static void prevLeakSpot();
	
	// Toggles visibility of the point file line
	static void toggle();

private:
	// Parse the current pointfile and read the vectors into the point list
	void parse();
};

} // namespace map

#endif /*POINTFILE_H_*/
