#pragma once

#include "igl.h"
#include "irender.h"
#include "editable.h"
#include "render.h"
#include "irenderable.h"
#include "math/Frustum.h"
#include "transformlib.h"
#include "scene/TransformedCopy.h"

#include "../OriginKey.h"
#include "../RotationKey.h"
#include "../ColourKey.h"
#include "../ModelKey.h"
#include "../SpawnArgs.h"
#include "../KeyObserverDelegate.h"

#include "Renderables.h"
#include "LightShader.h"
#include "RenderableVertices.h"
#include "Doom3LightRadius.h"

namespace entity {

void light_vertices(const AABB& aabb_light, Vector3 points[6]);
void light_draw(const AABB& aabb_light, RenderStateFlags state);

inline void default_extents(Vector3& extents) {
	extents = Vector3(8,8,8);
}

class LightNode;

/**
 * \brief
 * Main implementation of a light in the scene
 *
 * greebo: This is the actual light class. It contains the information about
 * the geometry of the light and the actual render functions.
 *
 * This class owns all the keyObserver callbacks, that get invoked as soon as
 * the entity key/values get changed by the user.
 *
 * The subclass Doom3LightRadius contains some variables like the light radius
 * and light center coordinates, and there are some "onChanged" callbacks for
 * the light radius and light center.
 *
 * Note: All the selection stuff is handled by the LightInstance class. This is
 * just the bare bone light.
 */
class Light: public RendererLight, public sigc::trackable
{
	friend class LightNode;

	LightNode& _owner;

    // The parent entity object that uses this light
	SpawnArgs& _entity;

	OriginKey m_originKey;
	// The "working" version of the origin
	Vector3 _originTransformed;

    RotationKey m_rotationKey;
    RotationMatrix m_rotation;

	Doom3LightRadius m_doom3Radius;

	// Renderable components of this light
	RenderableLightTarget _rCentre;
	RenderableLightTarget _rTarget;

	RenderableLightRelative _rUp;
	RenderableLightRelative _rRight;

	RenderableLightTarget _rStart;
	RenderableLightTarget _rEnd;

    RotationMatrix m_lightRotation;
    bool m_useLightRotation;

    // Set of values defining a projected light
    template<typename T> struct Projected
    {
        T target;
        T up;
        T right;
        T start;
        T end;
    };

    // Projected light vectors, both base and transformed
    scene::TransformedCopy<Projected<Vector3>> _projVectors;

    // Projected light vector colours
    Projected<Vector3> _projColours;

    // Projected light use flags
    Projected<bool> _projUseFlags;

    mutable AABB m_doom3AABB;
    mutable Matrix4 m_doom3Rotation;

    // Frustum for projected light (used for rendering the light volume)
    mutable Frustum _frustum;

    // Transforms local space coordinates into texture coordinates
    // To get the complete texture transform this one needs to be
    // post-multiplied by the world rotation and translation.
    mutable Matrix4 _localToTexture;

    mutable bool _projectionChanged;

	LightShader m_shader;

    // The 8x8 box representing the light object itself
    AABB _lightBox;

    Callback m_transformChanged;
    Callback m_boundsChanged;
    Callback m_evaluateTransform;

private:

	void construct();
	void destroy();

    // Ensure the start and end points are set to sensible values
	void checkStartEnd();

public:

    const Vector3& getUntransformedOrigin() const;

	void updateOrigin();

	void originChanged();

	void lightTargetChanged(const std::string& value);
	void lightUpChanged(const std::string& value);
	void lightRightChanged(const std::string& value);
	void lightStartChanged(const std::string& value);
	void lightEndChanged(const std::string& value);

	void writeLightOrigin();

	void rotationChanged();

	void lightRotationChanged(const std::string& value);

	/**
     * \brief
     * Main constructor.
     */
	Light(SpawnArgs& entity,
		  LightNode& owner,
          const Callback& transformChanged,
          const Callback& boundsChanged,
		  const Callback& lightRadiusChanged);

	/**
     * \brief
     * Copy constructor.
     */
	Light(const Light& other,
		  LightNode& owner,
          SpawnArgs& entity,
          const Callback& transformChanged,
          const Callback& boundsChanged,
		  const Callback& lightRadiusChanged);

	~Light();

	const AABB& localAABB() const;
	AABB lightAABB() const override;

	// Note: move this upwards
	mutable Matrix4 m_projectionOrientation;

	// Renderable submission functions
	void renderWireframe(RenderableCollector& collector,
						 const VolumeTest& volume,
						 const Matrix4& localToWorld,
						 bool selected) const;

	void setRenderSystem(const RenderSystemPtr& renderSystem);

	// Adds the light centre renderable to the given collector
	void renderLightCentre(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const;
	void renderProjectionPoints(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const;

	// Returns a reference to the member class Doom3LightRadius (used to set colours)
	Doom3LightRadius& getDoom3Radius();

	void translate(const Vector3& translation);

    /**
     * greebo: This sets the light start to the given value, including bounds checks.
     */
	void setLightStart(const Vector3& newLightStart);

    /**
     * greebo: Checks if the light_start is positioned "above" the light origin and constrains
     * the movement accordingly to prevent the light volume to become an "hourglass".
     * Only affects the _lightStartTransformed member.
     */
    void ensureLightStartConstraints();

	void rotate(const Quaternion& rotation);

	// This snaps the light as a whole to the grid (basically the light origin)
	void snapto(float snap);
	void setLightRadius(const AABB& aabb);
	void transformLightRadius(const Matrix4& transform);
	void revertTransform();
	void freezeTransform();

    // Is this light projected or omni?
    bool isProjected() const;

    // Set the projection-changed flag
	void projectionChanged();

    // Update the projected light frustum
    void updateProjection() const;

    // RendererLight implementation
    const IRenderEntity& getLightEntity() const override;
    Matrix4 getLightTextureTransformation() const override;
    Vector3 getLightOrigin() const override;
    const ShaderPtr& getShader() const override;

	const Matrix4& rotation() const;

	Vector3& target();
	Vector3& targetTransformed();
	Vector3& up();
	Vector3& upTransformed();
	Vector3& right();
	Vector3& rightTransformed();
	Vector3& start();
	Vector3& startTransformed();
	Vector3& end();
	Vector3& endTransformed();

	Vector3& colourLightTarget();
	Vector3& colourLightRight();
	Vector3& colourLightUp();
	Vector3& colourLightStart();
	Vector3& colourLightEnd();

	bool useStartEnd() const;

}; // class Light

} // namespace entity
