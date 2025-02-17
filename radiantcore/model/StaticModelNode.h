#pragma once

#include "Transformable.h"
#include "iselectiontest.h"
#include "irender.h"
#include "itraceable.h"
#include "modelskin.h"
#include "irenderable.h"
#include "pivot.h"
#include "render/VectorLightList.h"
#include "StaticModel.h"
#include "scene/Node.h"

namespace model {

/**
 * \brief Scenegraph node representing a static model loaded from a file (e.g.
 * LWO or ASE).
 *
 * This node does not represent a "func_static" (or similar object) directly,
 * but is added as a child of the respective entity node (e.g.
 * StaticGeometryNode). It is normally created by the ModelCache in response to
 * a particular entity gaining a "model" spawnarg.
 */
class StaticModelNode :
	public scene::Node,
	public ModelNode,
	public SelectionTestable,
	public SkinnedModel,
	public ITraceable,
    public Transformable
{
	// The actual model
	StaticModelPtr _model;

	std::string _name;

	// The name of this model's skin
	std::string _skin;

public:
    typedef std::shared_ptr<StaticModelNode> Ptr;

	/** Construct a StaticModelNode with a reference to the loaded picoModel.
	 */
	StaticModelNode(const StaticModelPtr& picoModel);

	virtual ~StaticModelNode();

	virtual void onInsertIntoScene(scene::IMapRootNode& root) override;
	virtual void onRemoveFromScene(scene::IMapRootNode& root) override;

	// ModelNode implementation
	const IModel& getIModel() const override;
	IModel& getIModel() override;
	bool hasModifiedScale() override;
	Vector3 getModelScale() override;

	// SkinnedModel implementation
	// Skin changed notify
	void skinChanged(const std::string& newSkinName) override;
	// Returns the name of the currently active skin
	std::string getSkin() const override;

	// Bounded implementation
	virtual const AABB& localAABB() const override;

	// SelectionTestable implementation
	void testSelect(Selector& selector, SelectionTest& test) override;

	virtual std::string name() const override;
	Type getNodeType() const override;

	const StaticModelPtr& getModel() const;
	void setModel(const StaticModelPtr& model);

	// Renderable implementation
  	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override;
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

	std::size_t getHighlightFlags() override
	{
		return Highlight::NoHighlight; // models are never highlighted themselves
	}

	// Traceable implementation
	bool getIntersection(const Ray& ray, Vector3& intersection) override;

protected:
	virtual void _onTransformationChanged() override;
	virtual void _applyTransformation() override;
};

} // namespace model
