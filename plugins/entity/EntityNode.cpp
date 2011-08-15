#include "EntityNode.h"

#include "EntitySettings.h"
#include "target/RenderableTargetInstances.h"

namespace entity {

EntityNode::EntityNode(const IEntityClassPtr& eclass) :
	TargetableNode(_entity, *this),
	_eclass(eclass),
	_entity(_eclass),
	_namespaceManager(_entity),
	_nameKey(_entity),
	_renderableName(_nameKey),
	_keyObservers(_entity)
{
	construct();
}

EntityNode::EntityNode(const EntityNode& other) :
	IEntityNode(other),
	SelectableNode(other),
	Namespaced(other),
	TargetableNode(_entity, *this),
	Nameable(other),
	Transformable(other),
	MatrixTransform(other),
	scene::Cloneable(other),
	IEntityClass::Observer(other),
	_eclass(other._eclass),
	_entity(other._entity),
	_namespaceManager(_entity),
	_nameKey(_entity),
	_renderableName(_nameKey),
	_keyObservers(_entity)
{
	construct();
}

EntityNode::~EntityNode()
{
	destruct();
}

void EntityNode::construct()
{
	_eclass->addObserver(this);

	TargetableNode::construct();

	addKeyObserver("name", _nameKey);
}

void EntityNode::destruct()
{
	removeKeyObserver("name", _nameKey);

	TargetableNode::destruct();

	_eclass->removeObserver(this);
}

void EntityNode::addKeyObserver(const std::string& key, KeyObserver& observer)
{
	_keyObservers.insert(key, observer);
}

void EntityNode::removeKeyObserver(const std::string& key, KeyObserver& observer)
{
	_keyObservers.erase(key, observer);
}

Entity& EntityNode::getEntity()
{
	return _entity;
}

float EntityNode::getShaderParm(int parmNum) const
{
	// FIXME: SLOW implementation - one could use an observer class holding 
	// the cached float value here

	// Entity parms 0-2 are mapped to the value of the _color spawnarg
	switch (parmNum)
	{
	case 0:
	case 1:
	case 2:
	{
		// Use a stringstream to parse the string
		std::string val = _entity.getKeyValue("_color");

		if (val.empty())
		{
			return 1.0f;
		}

		std::stringstream strm(val);

		Vector3 color;

		strm << std::skipws;
		strm >> color.x();
		strm >> color.y();
		strm >> color.z();

		switch (parmNum)
		{
		case 0: return color.x();
		case 1: return color.y();
		case 2: return color.z();
		};
		
		break;
	}
	default: 
		break;
	};
	
	// All other cases: look up the shaderParms from the dictionary
	std::string val = _entity.getKeyValue("shaderParm" + intToStr(parmNum));

	if (val.empty())
	{
		// parm3 is bound to "alpha" and defaults to 1.0
		return parmNum == 3 ? 1.0f : 0.0f;
	}
	else
	{
		return strToFloat(val);
	}
}

std::string EntityNode::getName() const {
	return _namespaceManager.getName();
}

void EntityNode::setNamespace(INamespace* space) {
	_namespaceManager.setNamespace(space);
}

INamespace* EntityNode::getNamespace() const {
	return _namespaceManager.getNamespace();
}

void EntityNode::connectNameObservers() {
	_namespaceManager.connectNameObservers();
}

void EntityNode::disconnectNameObservers() {
	_namespaceManager.disconnectNameObservers();
}

void EntityNode::attachNames() {
	_namespaceManager.attachNames();
}

void EntityNode::detachNames() {
	_namespaceManager.detachNames();
}

void EntityNode::changeName(const std::string& newName) {
	_namespaceManager.changeName(newName);
}

void EntityNode::onInsertIntoScene()
{
	_entity.instanceAttach(scene::findMapFile(getSelf()));

	// Register our TargetableNode, now that we're in the scene
	RenderableTargetInstances::Instance().attach(*this);

	SelectableNode::onInsertIntoScene();
}

void EntityNode::onRemoveFromScene()
{
	SelectableNode::onRemoveFromScene();

	RenderableTargetInstances::Instance().detach(*this);
	_entity.instanceDetach(scene::findMapFile(getSelf()));
}

std::string EntityNode::name() const
{
	return _nameKey.name();
}

void EntityNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	// Nothing so far (FIXME)
}

void EntityNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	// Submit renderable text name if required
	if (collector.getStyle() == RenderableCollector::eWireframeOnly &&
		EntitySettings::InstancePtr()->renderEntityNames())
    {
		collector.SetState(_entity.getEntityClass()->getWireShader(), RenderableCollector::eWireframeOnly);
		collector.addRenderable(_renderableName, localToWorld());
	}
}

bool EntityNode::isHighlighted() const
{
	return isSelected();
}

void EntityNode::OnEClassReload()
{
	// Let the keyobservers reload their values
	_keyObservers.refreshObservers();
}

} // namespace entity
