#pragma once

#include "Bounded.h"
#include "inode.h"
#include "iscenegraph.h"
#include "iselection.h"
#include "iselectable.h"
#include "itransformnode.h"
#include "itransformable.h"
#include "ientity.h"
#include "ipatch.h"
#include "ibrush.h"
#include "imodel.h"
#include "iparticlenode.h"

#include <cstddef>
#include <string.h>
#include <list>
#include <stack>

#include <boost/shared_ptr.hpp>

#include "scene/Node.h"

inline void Node_traverseSubgraph(const scene::INodePtr& node, scene::NodeVisitor& visitor) {
    if (node == NULL) return;

    // First, visit the node itself
    if (visitor.pre(node)) {
        // The walker requested to descend the children of this node as well,
        node->traverse(visitor);
    }

    visitor.post(node);
}

inline bool Node_isPrimitive(const scene::INodePtr& node)
{
    return Node_isBrush(node) || Node_isPatch(node);
}

class ParentBrushes :
    public scene::NodeVisitor
{
private:
    scene::INodePtr _parent;

public:
    ParentBrushes(const scene::INodePtr& parent) :
        _parent(parent)
    {}

    virtual bool pre(const scene::INodePtr& node)
    {
        return false;
    }

    virtual void post(const scene::INodePtr& node) 
    {
        if (Node_isPrimitive(node))
        {
            // We need to keep the hard reference to the node, such that the refcount doesn't reach 0
            scene::INodePtr nodeRef = node;

            scene::INodePtr oldParent = nodeRef->getParent();

            if (oldParent)
            {
                // greebo: remove the node from the old parent first
                oldParent->removeChildNode(nodeRef);
            }

            _parent->addChildNode(nodeRef);
        }
    }
};

inline void parentBrushes(const scene::INodePtr& subgraph, const scene::INodePtr& parent)
{
    ParentBrushes visitor(parent);
    subgraph->traverse(visitor);
}

namespace scene
{

/**
 * Returns true if the given node is a groupnode containing
 * child primitives. Being an entity is obviously not enough.
 */
inline bool isGroupNode(const INodePtr& node)
{
    // A node without child nodes is not a group
    if (!node->hasChildNodes())
	{
        return false;
    }

	bool hasBrushes = false;

	node->foreachNode([&] (const INodePtr& child)->bool
	{
		if (Node_isPrimitive(child))
		{
            hasBrushes = true;
			return false; // don't traverse any further
        }
		else
		{
			return true;
		}
	});

    return hasBrushes;
}

/**
 * greebo: This removes the given node from its parent node.
 *         The node is also deselected beforehand.
 */
inline void removeNodeFromParent(const INodePtr& node)
{
    // Check if the node has a parent in the first place
    INodePtr parent = node->getParent();

    if (parent != NULL)
	{
        // Unselect the node
        Node_setSelected(node, false);

        parent->removeChildNode(node);
    }
}

/**
 * greebo: This assigns the given node to the given set of layers. Any previous
 *         assignments of the node get overwritten by this routine.
 */
inline void assignNodeToLayers(const INodePtr& node, const LayerList& layers)
{
    if (!layers.empty())
    {
        LayerList::const_iterator i = layers.begin();

        // Move the node to the first layer (so that it gets removed from all others)
        node->moveToLayer(*i);

        // Add the node to all remaining layers
        for (++i; i != layers.end(); ++i)
        {
            node->addToLayer(*i);
        }
    }
}

/**
 * This assigns every visited node to the given set of layers.
 * Any previous assignments of the node get overwritten by this routine.
 */
class AssignNodeToLayersWalker :
    public NodeVisitor
{
    const LayerList& _layers;
public:
    AssignNodeToLayersWalker(const LayerList& layers) :
        _layers(layers)
    {}

    bool pre(const INodePtr& node) {
        // Pass the call to the single-node method
        assignNodeToLayers(node, _layers);

        return true; // full traverse
    }
};

class UpdateNodeVisibilityWalker :
    public NodeVisitor
{
    std::stack<bool> _visibilityStack;
public:
    bool pre(const INodePtr& node) {
        // Update the node visibility and store the result
        bool nodeIsVisible = GlobalLayerSystem().updateNodeVisibility(node);

        // Add a new element for this level
        _visibilityStack.push(nodeIsVisible);

        return true;
    }

    void post(const INodePtr& node) {
        // Is this child visible?
        bool childIsVisible = _visibilityStack.top();

        _visibilityStack.pop();

        if (childIsVisible) {
            // Show the node, regardless whether it was hidden before
            // otherwise the parent would hide the visible children as well
            node->disable(Node::eLayered);
        }

        if (!node->visible()) {
            // Node is hidden after update (and no children are visible), de-select
            Node_setSelected(node, false);
        }

        if (childIsVisible && !_visibilityStack.empty()) {
            // The child was visible, set this parent to true
            _visibilityStack.top() = true;
        }
    }
};

/**
 * greebo: This method inserts the given node into the given container
 *         and ensures that the container's layer visibility is updated.
 */
inline void addNodeToContainer(const INodePtr& node, const INodePtr& container) {
    // Insert the child
    container->addChildNode(node);

    // Ensure that worldspawn is visible
    UpdateNodeVisibilityWalker walker;
    Node_traverseSubgraph(container, walker);
}

} // namespace scene

inline ITransformablePtr Node_getTransformable(const scene::INodePtr& node) {
    return boost::dynamic_pointer_cast<ITransformable>(node);
}

// greebo: These tool methods have been moved from map.cpp, they might come in handy
enum ENodeType
{
    eNodeUnknown,
    eNodeMap,
    eNodeEntity,
    eNodePrimitive,
    eNodeModel,
    eNodeParticle,
};

inline std::string nodetype_get_name(ENodeType type)
{
    switch (type)
    {
    case eNodeMap: return "map";
    case eNodeEntity: return "entity";
    case eNodePrimitive: return "primitive";
    case eNodeModel: return "model";
    case eNodeParticle: return "particle";
    default: return "unknown";
    };
}

inline ENodeType node_get_nodetype(const scene::INodePtr& node)
{
    if (Node_isEntity(node)) {
        return eNodeEntity;
    }
    else if (Node_isPrimitive(node)) {
        return eNodePrimitive;
    }
    else if (Node_isModel(node)) {
        return eNodeModel;
    }
    else if (particles::isParticleNode(node))
    {
        return eNodeParticle;
    }
    return eNodeUnknown;
}

class SelectedDescendantWalker :
    public scene::NodeVisitor
{
    bool& m_selected;
public:
    SelectedDescendantWalker(bool& selected) :
        m_selected(selected)
    {
        m_selected = false;
    }

    virtual bool pre(const scene::INodePtr& node) {
        if (node->isRoot()) {
            return false;
        }

        if (Node_isSelected(node)) {
            m_selected = true;
        }

        return true;
    }
};

inline bool Node_selectedDescendant(const scene::INodePtr& node) {
    bool selected;

    SelectedDescendantWalker visitor(selected);
    Node_traverseSubgraph(node, visitor);

    return selected;
}

class NodePathFinder :
    public scene::NodeVisitor
{
    mutable scene::Path _path;

    // The node to find
    const scene::INodePtr _needle;
public:
    NodePathFinder(const scene::INodePtr& needle) :
        _needle(needle)
    {}

    bool pre(const scene::INodePtr& n)
    {
        scene::NodePtr node = boost::dynamic_pointer_cast<scene::Node>(n);

        if (node == _needle)
        {
            _path = node->getPath(); // found!
        }

        // Descend deeper if path is still empty
        return _path.empty();
    }

    const scene::Path& getPath() {
        return _path;
    }
};

// greebo: Returns the path for the given node (SLOW, traverses the scenegraph!)
inline scene::Path findPath(const scene::INodePtr& node)
{
    NodePathFinder finder(node);
    Node_traverseSubgraph(GlobalSceneGraph().root(), finder);
    return finder.getPath();
}

namespace scene {

/**
 * greebo: This walker removes all encountered child nodes without
 * traversing each node's children. This deselects all removed nodes as well.
 *
 * Use this to clear all children from a node:
 *
 * NodeRemover walker();
 * node->traverse(walker);
 */
class NodeRemover :
    public scene::NodeVisitor
{
public:
    bool pre(const INodePtr& node) {
        // Copy the node, the reference might point right to
        // the parent's container
        scene::INodePtr copy(node);

        removeNodeFromParent(copy);

        return false;
    }
};

} // namespace scene
