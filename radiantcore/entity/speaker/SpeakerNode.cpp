#include "SpeakerNode.h"
#include "../EntitySettings.h"

#include "math/Frustum.h"
#include <functional>

namespace entity
{

namespace
{
    const std::string KEY_S_MAXDISTANCE("s_maxdistance");
    const std::string KEY_S_MINDISTANCE("s_mindistance");
    const std::string KEY_S_SHADER("s_shader");
}

SpeakerNode::SpeakerNode(const IEntityClassPtr& eclass) :
	EntityNode(eclass),
	m_originKey(std::bind(&SpeakerNode::originChanged, this)),
	_renderableRadii(m_origin, _radiiTransformed),
	m_aabb_solid(m_aabb_local),
	m_aabb_wire(m_aabb_local),
	_dragPlanes(std::bind(&SpeakerNode::selectedChangedComponent, this, std::placeholders::_1))
{}

SpeakerNode::SpeakerNode(const SpeakerNode& other) :
	EntityNode(other),
	Snappable(other),
	m_originKey(std::bind(&SpeakerNode::originChanged, this)),
	_renderableRadii(m_origin, _radiiTransformed),
	m_aabb_solid(m_aabb_local),
	m_aabb_wire(m_aabb_local),
	_dragPlanes(std::bind(&SpeakerNode::selectedChangedComponent, this, std::placeholders::_1))
{}

std::shared_ptr<SpeakerNode> SpeakerNode::create(const IEntityClassPtr& eclass)
{
	SpeakerNodePtr speaker(new SpeakerNode(eclass));
	speaker->construct();

	return speaker;
}

void SpeakerNode::construct()
{
	EntityNode::construct();

	m_aabb_local = _spawnArgs.getEntityClass()->getBounds();
	m_aabb_border = m_aabb_local;

	observeKey("origin", sigc::mem_fun(m_originKey, &OriginKey::onKeyValueChanged));

    // Observe speaker-related spawnargs
    static_assert(std::is_base_of<sigc::trackable, SpeakerNode>::value);
	observeKey(KEY_S_SHADER, sigc::mem_fun(this, &SpeakerNode::sShaderChanged));
	observeKey(KEY_S_MINDISTANCE, sigc::mem_fun(this, &SpeakerNode::sMinChanged));
	observeKey(KEY_S_MAXDISTANCE, sigc::mem_fun(this, &SpeakerNode::sMaxChanged));
}

SpeakerNode::~SpeakerNode()
{
}

void SpeakerNode::originChanged()
{
	m_origin = m_originKey.get();
	updateTransform();
}

void SpeakerNode::sShaderChanged(const std::string& value)
{
	if (value.empty())
	{
		_defaultRadii.setMin(0);
		_defaultRadii.setMax(0);
	}
	else
	{
		if (module::RegistryReference::Instance().getRegistry().moduleExists(MODULE_SOUNDMANAGER))
		{
			// Non-zero shader set, retrieve the default radii
			_defaultRadii = GlobalSoundManager().getSoundShader(value)->getRadii();
		}
		else
		{
			_defaultRadii.setMin(0);
			_defaultRadii.setMax(0);
		}
	}

	// If we haven't overridden our distances yet, adjust these values to defaults
	if (!m_minIsSet)
	{
		_radii.setMin(_defaultRadii.getMin());
	}

	if (!m_maxIsSet)
	{
		_radii.setMax(_defaultRadii.getMax());
	}

	// Store the new values into our working set
	_radiiTransformed = _radii;

	updateAABB();
}

void SpeakerNode::sMinChanged(const std::string& value)
{
	// Check whether the spawnarg got set or removed
	m_minIsSet = value.empty() ? false : true;

	if (m_minIsSet)
	{
		// we need to parse in metres
		_radii.setMin(string::convert<float>(value), true);
	}
	else
	{
		_radii.setMin(_defaultRadii.getMin());
	}

	// Store the new value into our working set
	_radiiTransformed.setMin(_radii.getMin());

	updateAABB();
}

void SpeakerNode::sMaxChanged(const std::string& value)
{
	m_maxIsSet = value.empty() ? false : true;

	if (m_maxIsSet)
	{
		// we need to parse in metres
		_radii.setMax(string::convert<float>(value), true);
	}
	else
	{
		_radii.setMax(_defaultRadii.getMax());
	}

	// Store the new value into our working set
	_radiiTransformed.setMax(_radii.getMax());

	updateAABB();
}

// Snappable implementation
void SpeakerNode::snapto(float snap)
{
	m_originKey.snap(snap);
	m_originKey.write(_spawnArgs);
}

// Bounded implementation
const AABB& SpeakerNode::localAABB() const
{
	return m_aabb_border;
}

AABB SpeakerNode::getSpeakerAABB() const
{
    return AABB(m_originKey.get(), m_aabb_local.getExtents());
}

void SpeakerNode::selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback)
{
	test.BeginMesh(localToWorld());

	_dragPlanes.selectPlanes(localAABB(), selector, test, selectedPlaneCallback);
}

void SpeakerNode::selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes)
{
	_dragPlanes.selectReversedPlanes(localAABB(), selector, selectedPlanes);
}

void SpeakerNode::testSelect(Selector& selector, SelectionTest& test)
{
	EntityNode::testSelect(selector, test);

	test.BeginMesh(localToWorld());

	SelectionIntersection best;
	aabb_testselect(m_aabb_local, test, best);
	if(best.isValid()) {
		selector.addIntersection(best);
	}
}

void SpeakerNode::selectedChangedComponent(const ISelectable& selectable)
{
	// add the selectable to the list of selected components (see RadiantSelectionSystem::onComponentSelection)
	GlobalSelectionSystem().onComponentSelection(Node::getSelf(), selectable);
}

bool SpeakerNode::isSelectedComponents() const
{
	return _dragPlanes.isSelected();
}

void SpeakerNode::setSelectedComponents(bool select, selection::ComponentSelectionMode mode)
{
	if (mode == selection::ComponentSelectionMode::Face)
	{
		_dragPlanes.setSelected(false);
	}
}

void SpeakerNode::invertSelectedComponents(selection::ComponentSelectionMode mode)
{
	// nothing, planes are selected via selectPlanes()
}

void SpeakerNode::testSelectComponents(Selector& selector, SelectionTest& test, selection::ComponentSelectionMode mode)
{
	// nothing, planes are selected via selectPlanes()
}

scene::INodePtr SpeakerNode::clone() const
{
	SpeakerNodePtr node(new SpeakerNode(*this));
	node->construct();
    node->constructClone(*this);

	return node;
}

/* Renderable implementation */

void SpeakerNode::renderSolid(RenderableCollector& collector,
                              const VolumeTest& volume) const
{
	EntityNode::renderSolid(collector, volume);

	collector.addRenderable(*getFillShader(), m_aabb_solid, localToWorld());

    // Submit the speaker radius if we are selected or the "show all speaker
    // radii" option is set
	if (isSelected() || EntitySettings::InstancePtr()->getShowAllSpeakerRadii())
    {
		collector.addRenderable(*getFillShader(), _renderableRadii, localToWorld());
    }
}
void SpeakerNode::renderWireframe(RenderableCollector& collector,
                                  const VolumeTest& volume) const
{
	EntityNode::renderWireframe(collector, volume);

	collector.addRenderable(*getWireShader(), m_aabb_wire, localToWorld());

    // Submit the speaker radius if we are selected or the "show all speaker
    // radii" option is set
	if (isSelected() || EntitySettings::InstancePtr()->getShowAllSpeakerRadii())
    {
		collector.addRenderable(*getWireShader(), _renderableRadii, localToWorld());
    }
}

void SpeakerNode::translate(const Vector3& translation)
{
	m_origin += translation;
}

void SpeakerNode::updateTransform()
{
	localToParent() = Matrix4::getTranslation(m_origin);
	transformChanged();
}

void SpeakerNode::updateAABB()
{
	// set the AABB to the biggest AABB the speaker contains
	m_aabb_border = m_aabb_local;

	float radius = _radiiTransformed.getMax();
	m_aabb_border.extents = Vector3(radius, radius, radius);

	boundsChanged();
}

void SpeakerNode::setRadiusFromAABB(const AABB& aabb)
{
	// Find out which dimension got changed the most
	Vector3 delta = aabb.getExtents() - localAABB().getExtents();

	double maxTrans;

	// Get the maximum translation component
	if (fabs(delta.x()) > fabs(delta.y()))
	{
		maxTrans = (fabs(delta.x()) > fabs(delta.z())) ? delta.x() : delta.z();
	}
	else
	{
		maxTrans = (fabs(delta.y()) > fabs(delta.z())) ? delta.y() : delta.z();
	}

	if (EntitySettings::InstancePtr()->getDragResizeEntitiesSymmetrically())
	{
		// For a symmetric AABB change, take the extents delta times 2
		maxTrans *= 2;
	}
	else
	{
		// Update the origin accordingly
		m_origin += aabb.origin - localAABB().getOrigin();
	}

	float oldRadius = _radii.getMax() > 0 ? _radii.getMax() : _radii.getMin();

	// Prevent division by zero
	if (oldRadius == 0)
	{
		oldRadius = 1;
	}

	float newMax = static_cast<float>(oldRadius + maxTrans);

	float ratio = newMax / oldRadius;
	float newMin = _radii.getMin() * ratio;

	if (newMax < 0) newMax = 0.02f;
	if (newMin < 0) newMin = 0.01f;

	// Resize the radii and update the min radius proportionally
	_radiiTransformed.setMax(newMax);
	_radiiTransformed.setMin(newMin);

	updateAABB();
	updateTransform();
}

void SpeakerNode::evaluateTransform()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		translate(getTranslation());
	}
	else
	{
		// This seems to be a drag operation
		_dragPlanes.m_bounds = localAABB();

		// Let the dragplanes helper resize our local AABB
		AABB resizedAABB = _dragPlanes.evaluateResize(getTranslation(), Matrix4::getIdentity());

		setRadiusFromAABB(resizedAABB);
	}
}

void SpeakerNode::revertTransform()
{
	m_origin = m_originKey.get();

	_radiiTransformed = _radii;
}

void SpeakerNode::freezeTransform()
{
	m_originKey.set(m_origin);
	m_originKey.write(_spawnArgs);

	_radii = _radiiTransformed;

	// Write the s_mindistance/s_maxdistance keyvalues if we have a valid shader
	if (!_spawnArgs.getKeyValue(KEY_S_SHADER).empty())
	{
		// Note: Write the spawnargs in meters

		if (_radii.getMax() != _defaultRadii.getMax())
		{
			_spawnArgs.setKeyValue(KEY_S_MAXDISTANCE, string::to_string(_radii.getMax(true)));
		}
		else
		{
			// Radius is matching default, clear the spawnarg
			_spawnArgs.setKeyValue(KEY_S_MAXDISTANCE, "");
		}

		if (_radii.getMin() != _defaultRadii.getMin())
		{
			_spawnArgs.setKeyValue(KEY_S_MINDISTANCE, string::to_string(_radii.getMin(true)));
		}
		else
		{
			// Radius is matching default, clear the spawnarg
			_spawnArgs.setKeyValue(KEY_S_MINDISTANCE, "");
		}
	}
}

void SpeakerNode::_onTransformationChanged()
{
	revertTransform();
	evaluateTransform();
	updateTransform();
}

void SpeakerNode::_applyTransformation()
{
	revertTransform();
	evaluateTransform();
	freezeTransform();
}

} // namespace entity
