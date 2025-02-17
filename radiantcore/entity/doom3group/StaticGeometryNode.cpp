#include "StaticGeometryNode.h"

#include <functional>
#include "../curve/CurveControlPointFunctors.h"

#include "Translatable.h"

namespace entity
{

StaticGeometryNode::StaticGeometryNode(const IEntityClassPtr& eclass) :
	EntityNode(eclass),
	m_originKey(std::bind(&StaticGeometryNode::originChanged, this)),
	m_origin(ORIGINKEY_IDENTITY),
	m_nameOrigin(0,0,0),
	m_rotationKey(std::bind(&StaticGeometryNode::rotationChanged, this)),
	m_renderOrigin(m_nameOrigin),
	m_isModel(false),
	m_curveNURBS(std::bind(&scene::Node::boundsChanged, this)),
	m_curveCatmullRom(std::bind(&scene::Node::boundsChanged, this)),
	_nurbsEditInstance(m_curveNURBS,
				 std::bind(&StaticGeometryNode::selectionChangedComponent, this, std::placeholders::_1)),
	_catmullRomEditInstance(m_curveCatmullRom,
					  std::bind(&StaticGeometryNode::selectionChangedComponent, this, std::placeholders::_1)),
	_originInstance(VertexInstance(getOrigin(), std::bind(&StaticGeometryNode::selectionChangedComponent, this, std::placeholders::_1)))
{}

StaticGeometryNode::StaticGeometryNode(const StaticGeometryNode& other) :
	EntityNode(other),
	scene::GroupNode(other),
	Snappable(other),
	ComponentSelectionTestable(other),
	ComponentEditable(other),
	ComponentSnappable(other),
	CurveNode(other),
	m_originKey(std::bind(&StaticGeometryNode::originChanged, this)),
	m_origin(other.m_origin),
	m_nameOrigin(other.m_nameOrigin),
	m_rotationKey(std::bind(&StaticGeometryNode::rotationChanged, this)),
	m_renderOrigin(m_nameOrigin),
	m_isModel(other.m_isModel),
	m_curveNURBS(std::bind(&scene::Node::boundsChanged, this)),
	m_curveCatmullRom(std::bind(&scene::Node::boundsChanged, this)),
	_nurbsEditInstance(m_curveNURBS,
				 std::bind(&StaticGeometryNode::selectionChangedComponent, this, std::placeholders::_1)),
	_catmullRomEditInstance(m_curveCatmullRom,
					  std::bind(&StaticGeometryNode::selectionChangedComponent, this, std::placeholders::_1)),
	_originInstance(VertexInstance(getOrigin(), std::bind(&StaticGeometryNode::selectionChangedComponent, this, std::placeholders::_1)))
{
	// greebo: Don't call construct() here, this should be invoked by the
	// clone() method
}

StaticGeometryNode::Ptr StaticGeometryNode::Create(const IEntityClassPtr& eclass)
{
	StaticGeometryNode::Ptr instance(new StaticGeometryNode(eclass));
	instance->construct();

	return instance;
}

StaticGeometryNode::~StaticGeometryNode()
{
	destroy();
}

void StaticGeometryNode::construct()
{
    EntityNode::construct();

	m_rotation.setIdentity();

    // Observe common spawnarg changes
    static_assert(std::is_base_of<sigc::trackable, RotationKey>::value);
    static_assert(std::is_base_of<sigc::trackable, StaticGeometryNode>::value);
    observeKey("origin", sigc::mem_fun(m_originKey, &OriginKey::onKeyValueChanged));
    observeKey("angle", sigc::mem_fun(m_rotationKey, &RotationKey::angleChanged));
    observeKey("rotation", sigc::mem_fun(m_rotationKey, &RotationKey::rotationChanged));
    observeKey("name", sigc::mem_fun(this, &StaticGeometryNode::nameChanged));

    // Observe curve-related spawnargs
    static_assert(std::is_base_of<sigc::trackable, CurveNURBS>::value);
    static_assert(std::is_base_of<sigc::trackable, CurveCatmullRom>::value);
    observeKey(curve_Nurbs, sigc::mem_fun(m_curveNURBS, &CurveNURBS::onKeyValueChanged));
    observeKey(curve_CatmullRomSpline,
               sigc::mem_fun(m_curveCatmullRom, &CurveCatmullRom::onKeyValueChanged));

    updateIsModel();

    m_curveNURBS.signal_curveChanged().connect(
        sigc::mem_fun(_nurbsEditInstance, &CurveEditInstance::curveChanged)
    );
    m_curveCatmullRom.signal_curveChanged().connect(
        sigc::mem_fun(_catmullRomEditInstance, &CurveEditInstance::curveChanged)
    );
}

bool StaticGeometryNode::hasEmptyCurve() {
	return m_curveNURBS.isEmpty() &&
		   m_curveCatmullRom.isEmpty();
}

void StaticGeometryNode::removeSelectedControlPoints()
{
	if (_catmullRomEditInstance.isSelected()) {
		_catmullRomEditInstance.removeSelectedControlPoints();
		_catmullRomEditInstance.write(curve_CatmullRomSpline, _spawnArgs);
	}
	if (_nurbsEditInstance.isSelected()) {
		_nurbsEditInstance.removeSelectedControlPoints();
		_nurbsEditInstance.write(curve_Nurbs, _spawnArgs);
	}
}

void StaticGeometryNode::insertControlPointsAtSelected() {
	if (_catmullRomEditInstance.isSelected()) {
		_catmullRomEditInstance.insertControlPointsAtSelected();
		_catmullRomEditInstance.write(curve_CatmullRomSpline, _spawnArgs);
	}
	if (_nurbsEditInstance.isSelected()) {
		_nurbsEditInstance.insertControlPointsAtSelected();
		_nurbsEditInstance.write(curve_Nurbs, _spawnArgs);
	}
}

namespace
{

// Node visitor class to translate brushes
class BrushTranslator: public scene::NodeVisitor
{
    Vector3 m_origin;
public:
    BrushTranslator(const Vector3& origin) :
        m_origin(origin)
    {}

    bool pre(const scene::INodePtr& node)
    {
        Translatable* t = dynamic_cast<Translatable*>(node.get());
        if (t)
        {
            t->translate(m_origin);
        }
        return true;
    }
};

}

void StaticGeometryNode::addOriginToChildren()
{
	if (!isModel())
    {
		BrushTranslator translator(getOrigin());
		traverseChildren(translator);
	}
}

void StaticGeometryNode::removeOriginFromChildren()
{
	if (!isModel())
    {
		BrushTranslator translator(-getOrigin());
		traverseChildren(translator);
	}
}

void StaticGeometryNode::selectionChangedComponent(const ISelectable& selectable) {
	GlobalSelectionSystem().onComponentSelection(Node::getSelf(), selectable);
}

bool StaticGeometryNode::isSelectedComponents() const {
	return _nurbsEditInstance.isSelected() || _catmullRomEditInstance.isSelected() || (isModel() && _originInstance.isSelected());
}

void StaticGeometryNode::setSelectedComponents(bool selected, selection::ComponentSelectionMode mode) {
	if (mode == selection::ComponentSelectionMode::Vertex) {
		_nurbsEditInstance.setSelected(selected);
		_catmullRomEditInstance.setSelected(selected);
		_originInstance.setSelected(selected);
	}
}

void StaticGeometryNode::invertSelectedComponents(selection::ComponentSelectionMode mode)
{
	if (mode == selection::ComponentSelectionMode::Vertex)
	{
		_nurbsEditInstance.invertSelected();
		_catmullRomEditInstance.invertSelected();
		_originInstance.invertSelected();
	}
}

void StaticGeometryNode::testSelectComponents(Selector& selector, SelectionTest& test, selection::ComponentSelectionMode mode)
{
	if (mode == selection::ComponentSelectionMode::Vertex)
	{
		test.BeginMesh(localToWorld());

		_originInstance.testSelect(selector, test);

		_nurbsEditInstance.testSelect(selector, test);
		_catmullRomEditInstance.testSelect(selector, test);
	}
}

const AABB& StaticGeometryNode::getSelectedComponentsBounds() const {
	m_aabb_component = AABB();

	ControlPointBoundsAdder boundsAdder(m_aabb_component);
	_nurbsEditInstance.forEachSelected(boundsAdder);
	_catmullRomEditInstance.forEachSelected(boundsAdder);

	if (_originInstance.isSelected()) {
		m_aabb_component.includePoint(_originInstance.getVertex());
	}
	return m_aabb_component;
}

void StaticGeometryNode::snapComponents(float snap) {
	if (_nurbsEditInstance.isSelected()) {
		_nurbsEditInstance.snapto(snap);
		_nurbsEditInstance.write(curve_Nurbs, _spawnArgs);
	}
	if (_catmullRomEditInstance.isSelected()) {
		_catmullRomEditInstance.snapto(snap);
		_catmullRomEditInstance.write(curve_CatmullRomSpline, _spawnArgs);
	}
	if (_originInstance.isSelected()) {
		snapOrigin(snap);
	}
}

scene::INodePtr StaticGeometryNode::clone() const
{
	StaticGeometryNode::Ptr clone(new StaticGeometryNode(*this));
	clone->construct();
    clone->constructClone(*this);

	return clone;
}

void StaticGeometryNode::onRemoveFromScene(scene::IMapRootNode& root)
{
	// Call the base class first
	EntityNode::onRemoveFromScene(root);

	// De-select all child components as well
	setSelectedComponents(false, selection::ComponentSelectionMode::Vertex);
}

void StaticGeometryNode::testSelect(Selector& selector, SelectionTest& test)
{
	EntityNode::testSelect(selector, test);

	test.BeginMesh(localToWorld());
	SelectionIntersection best;

	// Pass the selection test to the StaticGeometryNode class
	m_curveNURBS.testSelect(selector, test, best);
	m_curveCatmullRom.testSelect(selector, test, best);

	// If the selectionIntersection is non-empty, add the selectable to the SelectionPool
	if (best.isValid()) {
		Selector_add(selector, *this, best);
	}
}

void StaticGeometryNode::renderCommon(RenderableCollector& collector, const VolumeTest& volume) const
{
	if (isSelected()) {
		m_renderOrigin.render(collector, volume, localToWorld());
	}

	if (!m_curveNURBS.isEmpty()) {
		// Always render curves relative to map origin
		m_curveNURBS.submitRenderables(getWireShader(), collector, volume, Matrix4::getIdentity());
	}

	if (!m_curveCatmullRom.isEmpty()) {
		// Always render curves relative to map origin
		m_curveCatmullRom.submitRenderables(getWireShader(), collector, volume, Matrix4::getIdentity());
	}
}

void StaticGeometryNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderSolid(collector, volume);
    renderCommon(collector, volume);

	// Render curves always relative to the absolute map origin
	_nurbsEditInstance.renderComponentsSelected(collector, volume, Matrix4::getIdentity());
	_catmullRomEditInstance.renderComponentsSelected(collector, volume, Matrix4::getIdentity());
}

void StaticGeometryNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderWireframe(collector, volume);
    renderCommon(collector, volume);

	_nurbsEditInstance.renderComponentsSelected(collector, volume, Matrix4::getIdentity());
	_catmullRomEditInstance.renderComponentsSelected(collector, volume, Matrix4::getIdentity());
}

void StaticGeometryNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	EntityNode::setRenderSystem(renderSystem);

	m_renderOrigin.setRenderSystem(renderSystem);
	_nurbsEditInstance.setRenderSystem(renderSystem);
	_catmullRomEditInstance.setRenderSystem(renderSystem);

	_originInstance.setRenderSystem(renderSystem);
}

void StaticGeometryNode::renderComponents(RenderableCollector& collector, const VolumeTest& volume) const
{
	if (GlobalSelectionSystem().ComponentMode() == selection::ComponentSelectionMode::Vertex)
	{
		_nurbsEditInstance.renderComponents(collector, volume, Matrix4::getIdentity());

		_catmullRomEditInstance.renderComponents(collector, volume, Matrix4::getIdentity());

		// Register the renderable with OpenGL
		if (!isModel()) {
			_originInstance.render(collector, volume, localToWorld());
		}
	}
}

void StaticGeometryNode::evaluateTransform()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		const Quaternion& rotation = getRotation();
		const Vector3& scaleFactor = getScale();

        rotate(rotation);
		scale(scaleFactor);
		translate(getTranslation());

		// Transform curve control points in primitive mode
		Matrix4 transformation = calculateTransform();
		_nurbsEditInstance.transform(transformation, false);
		_catmullRomEditInstance.transform(transformation, false);
	}
	else {
		// Transform the components only
		transformComponents(calculateTransform());
	}
	// Trigger a recalculation of the curve's controlpoints
	m_curveNURBS.curveChanged();
	m_curveCatmullRom.curveChanged();
}

void StaticGeometryNode::transformComponents(const Matrix4& matrix)
{
	if (_nurbsEditInstance.isSelected()) {
		_nurbsEditInstance.transform(matrix);
	}

	if (_catmullRomEditInstance.isSelected()) {
		_catmullRomEditInstance.transform(matrix);
	}

	if (_originInstance.isSelected()) {
		translateOrigin(getTranslation());
	}
}

void StaticGeometryNode::_onTransformationChanged()
{
	// If this is a container, pass the call to the children and leave the entity unharmed
	if (!isModel())
	{
		scene::forEachTransformable(*this, [] (ITransformable& child)
		{
			child.revertTransform();
		});

        revertTransformInternal();

		evaluateTransform();

		// Update the origin when we're in "child primitive" mode
		_renderableName.setOrigin(getOrigin());
	}
	else
	{
		// It's a model
		revertTransformInternal();
		evaluateTransform();
		updateTransform();
	}

	m_curveNURBS.curveChanged();
	m_curveCatmullRom.curveChanged();
}

void StaticGeometryNode::_applyTransformation()
{
	revertTransformInternal();
	evaluateTransform();
	freezeTransformInternal();

	if (!isModel())
	{
		// Update the origin when we're in "child primitive" mode
		_renderableName.setOrigin(getOrigin());
	}
}

void StaticGeometryNode::onModelKeyChanged(const std::string& value)
{
	// Override the default behaviour
	// Don't call EntityNode::onModelKeyChanged(value);

	// Pass the call to the contained model
	modelChanged(value);
}

inline void PointVertexArray_testSelect(VertexCb* first, std::size_t count,
	SelectionTest& test, SelectionIntersection& best)
{
	test.TestLineStrip(
	    VertexPointer(&first->vertex, sizeof(VertexCb)),
	    IndexPointer::index_type(count),
	    best
	);
}

Vector3& StaticGeometryNode::getOrigin() {
	return m_origin;
}

const Vector3& StaticGeometryNode::getUntransformedOrigin()
{
    return m_originKey.get();
}

const AABB& StaticGeometryNode::localAABB() const {
	m_curveBounds = m_curveNURBS.getBounds();
	m_curveBounds.includeAABB(m_curveCatmullRom.getBounds());

	if (m_curveBounds.isValid() || !m_isModel)
	{
		// Include the origin as well, it might be offset
		// Only do this, if the curve has valid bounds OR we have a non-Model,
		// otherwise we include the origin for models
		// and this AABB gets added to the children's
		// AABB in Instance::evaluateBounds(), which is wrong.
		m_curveBounds.includePoint(m_origin);
	}

	return m_curveBounds;
}

void StaticGeometryNode::snapOrigin(float snap)
{
	m_originKey.snap(snap);
	m_originKey.write(_spawnArgs);
	m_renderOrigin.updatePivot();
}

void StaticGeometryNode::translateOrigin(const Vector3& translation)
{
	m_origin = m_originKey.get() + translation;

	// Only non-models should have their rendered origin different than <0,0,0>
	if (!isModel())
	{
		m_nameOrigin = m_origin;
	}

	m_renderOrigin.updatePivot();
}

void StaticGeometryNode::translate(const Vector3& translation)
{
	m_origin += translation;

	// Only non-models should have their rendered origin different than <0,0,0>
	if (!isModel())
	{
		m_nameOrigin = m_origin;
	}

	m_renderOrigin.updatePivot();
	translateChildren(translation);
}

void StaticGeometryNode::rotate(const Quaternion& rotation)
{
	if (!isModel())
	{
		// Rotate all child nodes too
		scene::forEachTransformable(*this, [&] (ITransformable& child)
		{
			child.setType(TRANSFORM_PRIMITIVE);
			child.setRotation(rotation);
		});

        m_origin = rotation.transformPoint(m_origin);
        m_nameOrigin = m_origin;
        m_renderOrigin.updatePivot();
	}
	else
	{
		m_rotation.rotate(rotation);
	}
}

void StaticGeometryNode::scale(const Vector3& scale)
{
	if (!isModel())
	{
		// Scale all child nodes too
		scene::forEachTransformable(*this, [&] (ITransformable& child)
		{
			child.setType(TRANSFORM_PRIMITIVE);
			child.setScale(scale);
		});

        m_origin *= scale;
        m_nameOrigin = m_origin;
        m_renderOrigin.updatePivot();
	}
}

void StaticGeometryNode::snapto(float snap)
{
	m_originKey.snap(snap);
	m_originKey.write(_spawnArgs);
}

void StaticGeometryNode::revertTransformInternal()
{
	m_origin = m_originKey.get();

	// Only non-models should have their origin different than <0,0,0>
	if (!isModel()) {
		m_nameOrigin = m_origin;
	}
	else {
		m_rotation = m_rotationKey.m_rotation;
	}

	m_renderOrigin.updatePivot();
	m_curveNURBS.revertTransform();
	m_curveCatmullRom.revertTransform();
}

void StaticGeometryNode::freezeTransformInternal()
{
	m_originKey.set(m_origin);
	m_originKey.write(_spawnArgs);

	if (!isModel())
	{
		scene::forEachTransformable(*this, [] (ITransformable& child)
		{
			child.freezeTransform();
		});
	}
	else
	{
		m_rotationKey.m_rotation = m_rotation;
		m_rotationKey.write(&_spawnArgs, isModel());
	}

	m_curveNURBS.freezeTransform();
	m_curveNURBS.saveToEntity(_spawnArgs);

	m_curveCatmullRom.freezeTransform();
	m_curveCatmullRom.saveToEntity(_spawnArgs);
}

void StaticGeometryNode::appendControlPoints(unsigned int numPoints) {
	if (!m_curveNURBS.isEmpty()) {
		m_curveNURBS.appendControlPoints(numPoints);
		m_curveNURBS.saveToEntity(_spawnArgs);
	}
	if (!m_curveCatmullRom.isEmpty()) {
		m_curveCatmullRom.appendControlPoints(numPoints);
		m_curveCatmullRom.saveToEntity(_spawnArgs);
	}
}

void StaticGeometryNode::convertCurveType() {
	if (!m_curveNURBS.isEmpty() && m_curveCatmullRom.isEmpty()) {
		std::string keyValue = _spawnArgs.getKeyValue(curve_Nurbs);
		_spawnArgs.setKeyValue(curve_Nurbs, "");
		_spawnArgs.setKeyValue(curve_CatmullRomSpline, keyValue);
	}
	else if (!m_curveCatmullRom.isEmpty() && m_curveNURBS.isEmpty()) {
		std::string keyValue = _spawnArgs.getKeyValue(curve_CatmullRomSpline);
		_spawnArgs.setKeyValue(curve_CatmullRomSpline, "");
		_spawnArgs.setKeyValue(curve_Nurbs, keyValue);
	}
}

void StaticGeometryNode::destroy()
{
	modelChanged("");
}

bool StaticGeometryNode::isModel() const {
	return m_isModel;
}

void StaticGeometryNode::setIsModel(bool newValue) {
	if (newValue && !m_isModel) {
		// The model key is not recognised as "name"
		getModelKey().modelChanged(m_modelKey);
	}
	else if (!newValue && m_isModel) {
		// Clear the model path
		getModelKey().modelChanged("");
		m_nameOrigin = m_origin;
	}
	m_isModel = newValue;
	updateTransform();
}

/** Determine if this StaticGeometryNode is a model (func_static) or a
 * brush-containing entity. If the "model" key is equal to the
 * "name" key, then this is a brush-based entity, otherwise it is
 * a model entity. The exception to this is the "worldspawn"
 * entity class, which is always a brush-based entity.
 */
void StaticGeometryNode::updateIsModel()
{
	if (m_modelKey != m_name && !_spawnArgs.isWorldspawn())
	{
		setIsModel(true);

		// Set the renderable name back to 0,0,0
		_renderableName.setOrigin(Vector3(0,0,0));
	}
	else
	{
		setIsModel(false);

		// Update the renderable name
		_renderableName.setOrigin(getOrigin());
	}
}

void StaticGeometryNode::nameChanged(const std::string& value) {
	m_name = value;
	updateIsModel();
	m_renderOrigin.updatePivot();
}

void StaticGeometryNode::modelChanged(const std::string& value)
{
	m_modelKey = value;
	updateIsModel();

	if (isModel())
	{
		getModelKey().modelChanged(value);
		m_nameOrigin = Vector3(0,0,0);
	}
	else
	{
		getModelKey().modelChanged("");
		m_nameOrigin = m_origin;
	}

	m_renderOrigin.updatePivot();
}

void StaticGeometryNode::updateTransform()
{
	localToParent() = Matrix4::getIdentity();

	if (isModel())
	{
		localToParent().translateBy(m_origin);
		localToParent().multiplyBy(m_rotation.getMatrix4());
	}

	// Notify the Node about this transformation change	to update the local2World matrix
	transformChanged();
}

void StaticGeometryNode::translateChildren(const Vector3& childTranslation)
{
	if (inScene())
	{
		// Translate all child nodes too
		scene::forEachTransformable(*this, [&] (ITransformable& child)
		{
			child.setType(TRANSFORM_PRIMITIVE);
			child.setTranslation(childTranslation);
		});
	}
}

void StaticGeometryNode::originChanged()
{
	m_origin = m_originKey.get();
	updateTransform();
	// Only non-models should have their origin different than <0,0,0>
	if (!isModel())
	{
		m_nameOrigin = m_origin;
		// Update the renderable name
		_renderableName.setOrigin(getOrigin());
	}
	m_renderOrigin.updatePivot();
}

void StaticGeometryNode::rotationChanged() {
	m_rotation = m_rotationKey.m_rotation;
	updateTransform();
}

} // namespace entity
